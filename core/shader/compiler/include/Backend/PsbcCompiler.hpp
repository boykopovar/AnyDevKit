#pragma once

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
