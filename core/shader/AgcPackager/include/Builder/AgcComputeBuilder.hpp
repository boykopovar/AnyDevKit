#ifndef ANYDEVKIT_SHADER_AGCCOMPUTEBUILDER_HPP
#define ANYDEVKIT_SHADER_AGCCOMPUTEBUILDER_HPP

#include <libpsbc/psbc_compile.h>

namespace AnyDevKit {

class AgcComputeBuilder {
public:
    static PsbcShaderMetadata ReadMetadata(const PsbcShaderOutput& compiled);
};

}

#endif
