#include "ShaderCompiler.hpp"
#include <SceShaders.hpp>
#include <prx/libSceAgc/Shader/include/ShaderConstants.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::string read(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open shader source: " + path.string());
    std::string text{std::istreambuf_iterator<char>(input), {}};
    if (input.bad() || text.empty()) throw std::runtime_error("Cannot read shader source: " + path.string());
    return text;
}

void validate(const AnyDevKit::AgcShaderPackage& package, PsbcStage stage, ShaderRegs::ShaderBinaryType type, std::uint32_t programRegister) {
    const auto bytes = package.GetHeaderBytes();
    require(bytes.size() >= sizeof(Shader), "Truncated AGC header");
    Shader header;
    std::memcpy(&header, bytes.data(), sizeof(header));
    require(header.file_header == ShaderRegs::SHADER_FILE_HEADER_MAGIC && header.version == ShaderRegs::SHADER_VERSION, "Invalid AGC header identity");
    require(header.type == static_cast<std::uint8_t>(type), "Wrong hardware shader type");
    require(header.header_size == bytes.size() && header.shader_size == package.GetCodeBytes().size() && header.shader_size > 0 && header.shader_size % 4 == 0, "Invalid package sizes");
    require(package.GetBindingMetadata().source_stage == stage, "Wrong source stage");
    std::uint64_t displacement;
    std::memcpy(&displacement, bytes.data() + offsetof(Shader, sh_registers), sizeof(displacement));
    require(displacement <= bytes.size() - offsetof(Shader, sh_registers), "Invalid register displacement");
    const auto offset = offsetof(Shader, sh_registers) + displacement;
    require(header.num_sh_registers > 0 && header.num_sh_registers <= (bytes.size() - offset) / sizeof(ShaderRegister), "Register table exceeds header");
    bool found = false;
    for (std::size_t index = 0; index < header.num_sh_registers; ++index) {
        ShaderRegister entry;
        std::memcpy(&entry, bytes.data() + offset + index * sizeof(entry), sizeof(entry));
        if (entry.offset == programRegister) {
            require(!found && entry.value == 0, "Program address must occur once and remain unrelocated");
            found = true;
        }
    }
    require(found, "Missing program address register");
    std::cout << "header=" << bytes.size() << " code=" << package.GetCodeBytes().size() << '\n';
}

}

int main(int count, char** arguments) {
    try {
        if (count != 3) throw std::invalid_argument("Expected stage and shader source directory");
        const std::string stage(arguments[1]);
        const std::filesystem::path directory(arguments[2]);
        using AnyDevKit::ShaderCompiler;
        if (stage == "vertex" || stage == "fragment" || stage == "compute") {
            const auto kind = stage == "vertex" ? PSBC_STAGE_VERTEX : stage == "fragment" ? PSBC_STAGE_FRAGMENT : PSBC_STAGE_COMPUTE;
            auto options = ShaderCompiler::CreateOptions(kind);
            if (kind == PSBC_STAGE_COMPUTE) {
                options.descriptor_binding_count = 1;
                options.descriptor_bindings[0] = {0, 0, PSBC_DESCRIPTOR_STORAGE_BUFFER, 1, 0, 16};
            }
            const auto package = ShaderCompiler::CompileFile(directory / (stage == "vertex" ? "Triangle.vert" : stage == "fragment" ? "Triangle.frag" : "WriteStorage.comp"), options);
            validate(package, kind, stage == "vertex" ? ShaderRegs::ShaderBinaryType::Gs : stage == "fragment" ? ShaderRegs::ShaderBinaryType::Ps : ShaderRegs::ShaderBinaryType::Cs, stage == "vertex" ? ShaderRegs::SPI_SHADER_PGM_LO_ES : stage == "fragment" ? ShaderRegs::SPI_SHADER_PGM_LO_PS : ShaderRegs::COMPUTE_PGM_LO);
            if (kind == PSBC_STAGE_COMPUTE) require(package.GetBindingMetadata().descriptor_set0_valid && package.GetBindingMetadata().push_constants_valid, "Compute resource bindings disappeared");
        } else if (stage == "geometry" || stage == "export") {
            const auto vertex = read(directory / (stage == "export" ? "GeometryExport.vert" : "Triangle.vert"));
            const auto geometry = read(directory / "EmitTriangle.geom");
            const auto package = ShaderCompiler::CompileGeometry({vertex, "Triangle.vert"}, {geometry, "EmitTriangle.geom"}, ShaderCompiler::CreateOptions(PSBC_STAGE_GEOMETRY), {});
            validate(package, PSBC_STAGE_GEOMETRY, ShaderRegs::ShaderBinaryType::Gs, ShaderRegs::SPI_SHADER_PGM_LO_ES);
            if (stage == "export") require(package.GetBindingMetadata().push_use_valid && package.GetBindingMetadata().previous_stage_push_dwords != 0, "Export stage push constants disappeared");
            require(package.GetBindingMetadata().merged_geometry && package.GetBindingMetadata().merged_es_source_stage == PSBC_STAGE_VERTEX, "Geometry is missing its export stage");
        } else if (stage == "tessellation_control" || stage == "tessellation_evaluation" || stage == "local") {
            const auto vertex = read(directory / (stage == "local" ? "TessellationLocal.vert" : "Triangle.vert"));
            const auto control = read(directory / "TrianglePatch.tesc");
            const auto evaluation = read(directory / "TrianglePatch.tese");
            auto options = ShaderCompiler::CreateOptions(PSBC_STAGE_TESS_CTRL);
            options.patch_control_points = 3;
            const auto packages = ShaderCompiler::CompileTessellation({vertex, "Triangle.vert"}, {control, "TrianglePatch.tesc"}, {evaluation, "TrianglePatch.tese"}, options);
            if (stage == "tessellation_control" || stage == "local") {
                validate(packages[0], PSBC_STAGE_TESS_CTRL, ShaderRegs::ShaderBinaryType::Hs, ShaderRegs::SPI_SHADER_PGM_LO_LS);
                if (stage == "local") require(packages[0].GetBindingMetadata().hull_vertex_push_dwords != 0, "Local stage push constants disappeared");
                require(packages[0].GetBindingMetadata().hull_tess_wg_valid, "Hull is missing its merged local stage layout");
            } else validate(packages[1], PSBC_STAGE_TESS_EVAL, ShaderRegs::ShaderBinaryType::Gs, ShaderRegs::SPI_SHADER_PGM_LO_ES);
        } else throw std::invalid_argument("Unknown stage");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
