#ifndef ANYDEVKIT_CORE_SHADER_AGCPACKAGER_INCLUDE_BUILDER_AGCRESOURCEBUILDER_HPP
#define ANYDEVKIT_CORE_SHADER_AGCPACKAGER_INCLUDE_BUILDER_AGCRESOURCEBUILDER_HPP

#include <libpsbc/psbc_compile.h>
#include "Writer/AgcBinaryWriter.hpp"

namespace AnyDevKit {

class AgcResourceBuilder {
public:
    static void Build(const PsbcShaderMetadata& metadata, AgcBinaryWriter& writer);
};

}

#endif
