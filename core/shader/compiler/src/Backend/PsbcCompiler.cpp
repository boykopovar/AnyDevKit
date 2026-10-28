#include "Backend/PsbcCompiler.hpp"

#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>

namespace AnyDevKit {

namespace {

std::mutex& compilerMutex() {
    static std::mutex mutex;
    return mutex;
}

void validateSpirv(std::span<const std::uint32_t> spirv) {
    if (spirv.size() < 5 || spirv.front() != 0x07230203u || spirv[4] != 0) throw std::invalid_argument("PSBC: invalid SPIR-V header");
    for (std::size_t offset = 5; offset < spirv.size();) {
        const auto count = spirv[offset] >> 16;
        if (count == 0 || count > spirv.size() - offset) throw std::invalid_argument("PSBC: invalid SPIR-V instruction length");
        offset += count;
    }
}

class PsbcRuntime {
public:
    PsbcRuntime() { psbc_init(); }
    ~PsbcRuntime() { psbc_shutdown(); }
    PsbcRuntime(const PsbcRuntime&) = delete;
    PsbcRuntime& operator=(const PsbcRuntime&) = delete;
};

class PsbcOutput {
public:
    PsbcOutput() = default;
    ~PsbcOutput() { psbc_free_output(&value); }
    PsbcOutput(const PsbcOutput&) = delete;
    PsbcOutput& operator=(const PsbcOutput&) = delete;
    PsbcShaderOutput& Get() { return value; }

private:
    PsbcShaderOutput value{};
};

}

void PsbcCompiler::ValidateOptions(const PsbcCompileOptions& options) {
    if (options.target != PSBC_TARGET_PS5 || options.address32_hi != 2) throw std::invalid_argument("PSBC: AGC requires the PS5 target and address32_hi=2");
    if (options.stage != PSBC_STAGE_VERTEX && options.stage != PSBC_STAGE_FRAGMENT && options.stage != PSBC_STAGE_GEOMETRY && options.stage != PSBC_STAGE_COMPUTE && options.stage != PSBC_STAGE_TESS_CTRL && options.stage != PSBC_STAGE_TESS_EVAL) throw std::invalid_argument("PSBC: unsupported AGC stage");
    if ((options.stage == PSBC_STAGE_VERTEX || options.stage == PSBC_STAGE_GEOMETRY || options.stage == PSBC_STAGE_TESS_EVAL) && !options.ngg) throw std::invalid_argument("PSBC: vertex AGC shaders require NGG");
    if ((options.stage == PSBC_STAGE_FRAGMENT || options.stage == PSBC_STAGE_COMPUTE) && (options.ngg || options.vertex_attribute_count)) throw std::invalid_argument("PSBC: vertex-only options on a fragment shader");
    const bool tessellation = options.stage == PSBC_STAGE_TESS_CTRL || options.stage == PSBC_STAGE_TESS_EVAL;
    if (tessellation && (!options.patch_control_points || options.patch_control_points > 32)) throw std::invalid_argument("PSBC: tessellation requires 1 to 32 patch control points");
    if (!tessellation && options.patch_control_points) throw std::invalid_argument("PSBC: patch control points on a non-tessellation stage");
    if (options.stage == PSBC_STAGE_TESS_CTRL && options.ngg) throw std::invalid_argument("PSBC: hull is not an NGG stage");
    if (!options.entrypoint || std::strcmp(options.entrypoint, "main") != 0) throw std::invalid_argument("PSBC: GLSL entry point must be main");
    if (!options.force_indirect_push_constants || !options.static_descriptor_use) throw std::invalid_argument("PSBC: indirect push constants and static descriptor usage are required");
    if ((!tessellation && options.primitive_type == 0) || options.primitive_type > 13 || (options.primitive_type > 6 && options.primitive_type < 10)) throw std::invalid_argument("PSBC: an explicit supported primitive topology is required");
    if (options.rasterization_samples != 1 && options.rasterization_samples != 2 && options.rasterization_samples != 4 && options.rasterization_samples != 8) throw std::invalid_argument("PSBC: sample count must be 1, 2, 4 or 8");
    if (options.stage == PSBC_STAGE_FRAGMENT && options.spi_shader_col_format == 0) throw std::invalid_argument("PSBC: an explicit color export format is required");
    if (options.force_accelerated_dot) throw std::invalid_argument("PSBC: diagnostic accelerated-dot mode is unsupported");
    if (options.vertex_attribute_count > PSBC_MAX_VERTEX_ATTRIBUTES || options.descriptor_binding_count > PSBC_MAX_DESCRIPTOR_BINDINGS || options.specialization_constant_count > PSBC_MAX_SPECIALIZATION_CONSTANTS) throw std::invalid_argument("PSBC: option table exceeds capacity");
    if (options.previous_parameters.enabled || options.next_link_parameters.enabled || options.ps5_global_streamout || options.fragment_distance_layout_valid || options.fragment_clip_distance_count || options.ngg_uses_scratch) throw std::invalid_argument("PSBC: unsupported linked-stage, streamout, distance or scratch options");
    if (options.ngg_device_facts && (!options.ngg_pc_lines || !options.ngg_min_good_cu_per_sa)) throw std::invalid_argument("PSBC: incomplete NGG device facts");
    if (!options.ngg_device_facts && (options.ngg_pc_lines || options.ngg_min_good_cu_per_sa)) throw std::invalid_argument("PSBC: NGG device facts supplied without enabling them");
    for (std::uint32_t index = 0; index < options.vertex_attribute_count; ++index) {
        const auto& attribute = options.vertex_attributes[index];
        if (attribute.location >= PSBC_MAX_VERTEX_ATTRIBUTES || attribute.binding >= PSBC_MAX_VERTEX_ATTRIBUTES || attribute.format <= PSBC_VERTEX_FORMAT_NONE || attribute.format > PSBC_VERTEX_FORMAT_R16G16B16A16_FLOAT || !attribute.alignment || (attribute.alignment & (attribute.alignment - 1))) throw std::invalid_argument("PSBC: invalid vertex attribute");
        for (std::uint32_t previous = 0; previous < index; ++previous) {
            const auto& other = options.vertex_attributes[previous];
            if (attribute.location == other.location) throw std::invalid_argument("PSBC: duplicate vertex attribute location");
            if (attribute.binding == other.binding && (attribute.stride != other.stride || attribute.instance_divisor != other.instance_divisor)) throw std::invalid_argument("PSBC: inconsistent vertex binding layout");
        }
    }
    for (std::uint32_t index = 0; index < options.specialization_constant_count; ++index) {
        const auto& entry = options.specialization_constants[index];
        if (entry.size != 1 && entry.size != 2 && entry.size != 4 && entry.size != 8) throw std::invalid_argument("PSBC: invalid specialization constant size");
        for (std::uint32_t previous = 0; previous < index; ++previous) {
            if (entry.constant_id == options.specialization_constants[previous].constant_id) throw std::invalid_argument("PSBC: duplicate specialization constant ID");
        }
    }
}

AgcShaderPackage PsbcCompiler::Compile(std::span<const std::uint32_t> spirv, const PsbcCompileOptions& options) {
    ValidateOptions(options);
    if (options.stage == PSBC_STAGE_TESS_CTRL || options.stage == PSBC_STAGE_TESS_EVAL) throw std::invalid_argument("PSBC: tessellation requires CompileTessellation and all three stages");
    if (options.stage == PSBC_STAGE_GEOMETRY) throw std::invalid_argument("PSBC: geometry requires CompileGeometry and its vertex stage");
    validateSpirv(spirv);
    const std::lock_guard lock(compilerMutex());
    static const PsbcRuntime runtime;
    PsbcOutput output;
    const auto result = psbc_compile_shader(spirv.data(), spirv.size_bytes(), &options, &output.Get());
    if (result != PSBC_RESULT_OK) throw std::runtime_error("PSBC: " + std::string(psbc_result_string(result)));
    if (output.Get().metadata.source_stage != options.stage) throw std::runtime_error("PSBC: compiled stage does not match the request");
    return AgcPackager::Build(output.Get());
}

AgcShaderPackage PsbcCompiler::CompileGeometry(std::span<const std::uint32_t> vertex, std::span<const std::uint32_t> geometry, const PsbcCompileOptions& options) {
    ValidateOptions(options);
    if (options.stage != PSBC_STAGE_GEOMETRY) throw std::invalid_argument("PSBC: geometry pipeline requires geometry options");
    validateSpirv(vertex);
    validateSpirv(geometry);
    const std::lock_guard lock(compilerMutex());
    static const PsbcRuntime runtime;
    PsbcOutput output;
    const auto result = psbc_compile_geometry_pipeline(vertex.data(), vertex.size_bytes(), geometry.data(), geometry.size_bytes(), &options, &output.Get());
    if (result != PSBC_RESULT_OK) throw std::runtime_error("PSBC: " + std::string(psbc_result_string(result)));
    if (output.Get().metadata.source_stage != PSBC_STAGE_GEOMETRY || !output.Get().metadata.merged_geometry) throw std::runtime_error("PSBC: missing merged geometry program");
    return AgcPackager::Build(output.Get());
}

std::array<AgcShaderPackage, 2> PsbcCompiler::CompileTessellation(std::span<const std::uint32_t> vertex, std::span<const std::uint32_t> control, std::span<const std::uint32_t> evaluation, const PsbcCompileOptions& options) {
    ValidateOptions(options);
    if (options.stage != PSBC_STAGE_TESS_CTRL) throw std::invalid_argument("PSBC: tessellation pipeline requires control options");
    validateSpirv(vertex);
    validateSpirv(control);
    validateSpirv(evaluation);
    const std::lock_guard lock(compilerMutex());
    static const PsbcRuntime runtime;
    PsbcOutput hull;
    auto result = psbc_compile_tess_pipeline(vertex.data(), vertex.size_bytes(), control.data(), control.size_bytes(), evaluation.data(), evaluation.size_bytes(), &options, &hull.Get());
    if (result != PSBC_RESULT_OK) throw std::runtime_error("PSBC hull: " + std::string(psbc_result_string(result)));
    auto domainOptions = options;
    domainOptions.stage = PSBC_STAGE_TESS_EVAL;
    domainOptions.ngg = true;
    PsbcOutput domain;
    result = psbc_compile_domain_pipeline(control.data(), control.size_bytes(), evaluation.data(), evaluation.size_bytes(), &domainOptions, &domain.Get());
    if (result != PSBC_RESULT_OK) throw std::runtime_error("PSBC domain: " + std::string(psbc_result_string(result)));
    if (hull.Get().metadata.hardware_stage != PSBC_HW_STAGE_HULL || domain.Get().metadata.source_stage != PSBC_STAGE_TESS_EVAL) throw std::runtime_error("PSBC: incomplete tessellation pipeline");
    return {AgcPackager::Build(hull.Get()), AgcPackager::Build(domain.Get())};
}

}
