#ifndef ANYDEVKIT_CORE_SHADER_AGCPACKAGER_INCLUDE_BUILDER_AGCSEMANTICBUILDER_HPP
#define ANYDEVKIT_CORE_SHADER_AGCPACKAGER_INCLUDE_BUILDER_AGCSEMANTICBUILDER_HPP

#include <libpsbc/psbc_compile.h>
#include "Writer/AgcBinaryWriter.hpp"

namespace AnyDevKit {

class AgcSemanticBuilder {
public:
    static void Build(const PsbcShaderMetadata& metadata, AgcBinaryWriter& writer);
};

}

#endif
