#ifndef ANYDEVKIT_CORE_SHADER_COMPILER_INCLUDE_BACKEND_PSBCCOMPILER_HPP
#define ANYDEVKIT_CORE_SHADER_COMPILER_INCLUDE_BACKEND_PSBCCOMPILER_HPP

#include <AgcPackager.hpp>
#include <cstdint>
#include <span>
#include <array>

namespace AnyDevKit {

class PsbcCompiler {
public:
    static void ValidateOptions(const PsbcCompileOptions& options);
    static std::array<AgcShaderPackage, 2> CompileTessellation(std::span<const std::uint32_t> vertex, std::span<const std::uint32_t> control, std::span<const std::uint32_t> evaluation, const PsbcCompileOptions& options);
    static AgcShaderPackage CompileGeometry(std::span<const std::uint32_t> vertex, std::span<const std::uint32_t> geometry, const PsbcCompileOptions& options);
    static AgcShaderPackage Compile(std::span<const std::uint32_t> spirv, const PsbcCompileOptions& options);
};

}

#endif
