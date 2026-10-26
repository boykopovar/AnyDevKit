#ifndef ANYDEVKIT_CORE_SHADER_COMPILER_INCLUDE_FRONTEND_SHADERINCLUDER_HPP
#define ANYDEVKIT_CORE_SHADER_COMPILER_INCLUDE_FRONTEND_SHADERINCLUDER_HPP

#include <filesystem>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <glslang/Public/ShaderLang.h>

namespace AnyDevKit {

class ShaderIncluder final : public glslang::TShader::Includer {
public:
    explicit ShaderIncluder(std::span<const std::filesystem::path> includeDirectories);
    static std::string ReadFile(const std::filesystem::path& path);

private:
    IncludeResult* includeSystem(const char* headerName, const char* includerName, std::size_t inclusionDepth) override;
    IncludeResult* includeLocal(const char* headerName, const char* includerName, std::size_t inclusionDepth) override;
    void releaseInclude(IncludeResult* result) override;
    IncludeResult* resolve(const char* headerName, const char* includerName, std::size_t inclusionDepth, bool local);
    std::vector<std::filesystem::path> includeDirectories;
    std::map<IncludeResult*, std::pair<std::unique_ptr<std::string>, std::unique_ptr<IncludeResult>>> includes;
};

}

#endif
