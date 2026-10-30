#ifndef ANYDEVKIT_CORE_SHADER_COMPILER_INCLUDE_SHADERCOMPILER_HPP
#define ANYDEVKIT_CORE_SHADER_COMPILER_INCLUDE_SHADERCOMPILER_HPP

#include <AgcPackager.hpp>
#include <filesystem>
#include <span>
#include <array>
#include <string_view>

namespace AnyDevKit {

struct ShaderSource {
    std::string_view Text;
    std::string_view Name;
};

class ShaderCompiler {
public:
    static PsbcCompileOptions CreateOptions(PsbcStage stage, std::string_view target = "ps5");
    static AgcShaderPackage Compile(std::string_view source, std::string_view sourceName, const PsbcCompileOptions& options, std::span<const std::filesystem::path> includeDirectories = {});
    static std::array<AgcShaderPackage, 2> CompileTessellation(const ShaderSource& vertex, const ShaderSource& control, const ShaderSource& evaluation, const PsbcCompileOptions& options, std::span<const std::filesystem::path> includeDirectories = {});
    static AgcShaderPackage CompileGeometry(const ShaderSource& vertex, const ShaderSource& geometry, const PsbcCompileOptions& options, std::span<const std::filesystem::path> includeDirectories = {});
    static AgcShaderPackage CompileFile(const std::filesystem::path& sourcePath, const PsbcCompileOptions& options, std::span<const std::filesystem::path> includeDirectories = {});
};

}

#endif
