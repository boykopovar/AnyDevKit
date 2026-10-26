#ifndef ANYDEVKIT_CORE_SHADER_COMPILER_INCLUDE_BACKEND_PSBCCOMPILER_HPP
#define ANYDEVKIT_CORE_SHADER_COMPILER_INCLUDE_BACKEND_PSBCCOMPILER_HPP

#include <AgcPackager.hpp>
#include <cstdint>
#include <span>

namespace AnyDevKit {

class PsbcCompiler {
public:
    static void ValidateOptions(const PsbcCompileOptions& options);
    static AgcShaderPackage Compile(std::span<const std::uint32_t> spirv, const PsbcCompileOptions& options);
};

}

#endif
