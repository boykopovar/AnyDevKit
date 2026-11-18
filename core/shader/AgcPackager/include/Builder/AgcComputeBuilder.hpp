#ifndef ANYDEVKIT_SHADER_AGCCOMPUTEBUILDER_HPP
#define ANYDEVKIT_SHADER_AGCCOMPUTEBUILDER_HPP

#include <cstdint>
#include <libpsbc/psbc_compile.h>

namespace AnyDevKit {

class AgcComputeBuilder {
public:
    static PsbcShaderMetadata ReadMetadata(const PsbcShaderOutput& compiled);
    static std::uint16_t ScratchDwordsPerThread(const PsbcShaderMetadata& metadata);
};

}

#endif

