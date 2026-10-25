#pragma once

#include <AgcPackager.hpp>
#include <filesystem>
#include <span>
#include <string_view>

namespace AnyDevKit {

class ShaderCompiler {
public:
    static PsbcCompileOptions CreateOptions(PsbcStage stage);
    static AgcShaderPackage Compile(std::string_view source, std::string_view sourceName, const PsbcCompileOptions& options, std::span<const std::filesystem::path> includeDirectories = {});
    static AgcShaderPackage CompileFile(const std::filesystem::path& sourcePath, const PsbcCompileOptions& options, std::span<const std::filesystem::path> includeDirectories = {});
};

}
