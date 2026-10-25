#pragma once

#include <cstddef>
#include <span>
#include <vector>
#include <libpsbc/psbc_compile.h>

namespace AnyDevKit {

class AgcShaderPackage {
public:
    static constexpr std::size_t HeaderAlignment = 8;
    static constexpr std::size_t CodeAlignment = 256;

    std::span<const std::byte> GetHeaderBytes() const;
    std::span<const std::byte> GetCodeBytes() const;
    const PsbcShaderMetadata& GetBindingMetadata() const;

private:
    friend class AgcPackager;
    AgcShaderPackage(std::vector<std::byte> headerBytes, std::vector<std::byte> codeBytes, const PsbcShaderMetadata& metadata);
    std::vector<std::byte> headerBytes;
    std::vector<std::byte> codeBytes;
    PsbcShaderMetadata metadata;
};

class AgcPackager {
public:
    static AgcShaderPackage Build(const PsbcShaderOutput& compiled);
};

}
