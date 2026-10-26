#ifndef ANYDEVKIT_CORE_SHADER_COMPILER_INCLUDE_FRONTEND_GLSLCOMPILER_HPP
#define ANYDEVKIT_CORE_SHADER_COMPILER_INCLUDE_FRONTEND_GLSLCOMPILER_HPP

#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>
#include <libpsbc/psbc_compile.h>

namespace AnyDevKit {

class GlslCompiler {
public:
    static std::vector<std::uint32_t> Compile(std::string_view source, std::string_view sourceName, const PsbcCompileOptions& options, std::span<const std::filesystem::path> includeDirectories);
};

}

#endif
