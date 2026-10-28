#include "ShaderCompiler.hpp"

#include "Backend/PsbcCompiler.hpp"
#include "Frontend/GlslCompiler.hpp"
#include "Frontend/ShaderIncluder.hpp"
#include <stdexcept>

namespace AnyDevKit {

PsbcCompileOptions ShaderCompiler::CreateOptions(PsbcStage stage) {
    if (stage != PSBC_STAGE_VERTEX && stage != PSBC_STAGE_FRAGMENT && stage != PSBC_STAGE_GEOMETRY && stage != PSBC_STAGE_COMPUTE) throw std::invalid_argument("Compiler: unsupported AGC stage");
    PsbcCompileOptions options{};
    options.target = PSBC_TARGET_PS5;
    options.stage = stage;
    options.entrypoint = "main";
    options.optimise = true;
    options.ngg = stage == PSBC_STAGE_VERTEX || stage == PSBC_STAGE_GEOMETRY;
    options.primitive_type = 4;
    options.address32_hi = 2;
    options.force_indirect_push_constants = true;
    options.static_descriptor_use = true;
    options.rasterization_samples = 1;
    options.spi_shader_col_format = stage == PSBC_STAGE_FRAGMENT ? 9 : 0;
    return options;
}

AgcShaderPackage ShaderCompiler::Compile(std::string_view source, std::string_view sourceName, const PsbcCompileOptions& options, std::span<const std::filesystem::path> includeDirectories) {
    PsbcCompiler::ValidateOptions(options);
    const auto spirv = GlslCompiler::Compile(source, sourceName, options, includeDirectories);
    return PsbcCompiler::Compile(spirv, options);
}

AgcShaderPackage ShaderCompiler::CompileGeometry(const ShaderSource& vertex, const ShaderSource& geometry, const PsbcCompileOptions& options, std::span<const std::filesystem::path> includeDirectories) {
    PsbcCompiler::ValidateOptions(options);
    if (options.stage != PSBC_STAGE_GEOMETRY) throw std::invalid_argument("Compiler: geometry pipeline requires geometry options");
    auto vertexOptions = options;
    vertexOptions.stage = PSBC_STAGE_VERTEX;
    const auto vertexSpirv = GlslCompiler::Compile(vertex.Text, vertex.Name, vertexOptions, includeDirectories);
    const auto geometrySpirv = GlslCompiler::Compile(geometry.Text, geometry.Name, options, includeDirectories);
    return PsbcCompiler::CompileGeometry(vertexSpirv, geometrySpirv, options);
}

AgcShaderPackage ShaderCompiler::CompileFile(const std::filesystem::path& sourcePath, const PsbcCompileOptions& options, std::span<const std::filesystem::path> includeDirectories) {
    PsbcCompiler::ValidateOptions(options);
    const auto path = std::filesystem::canonical(sourcePath);
    const auto source = ShaderIncluder::ReadFile(path);
    return Compile(source, path.generic_string(), options, includeDirectories);
}

}
