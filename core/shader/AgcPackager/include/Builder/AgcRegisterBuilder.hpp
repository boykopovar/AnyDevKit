#ifndef ANYDEVKIT_CORE_SHADER_AGCPACKAGER_INCLUDE_BUILDER_AGCREGISTERBUILDER_HPP
#define ANYDEVKIT_CORE_SHADER_AGCPACKAGER_INCLUDE_BUILDER_AGCREGISTERBUILDER_HPP

#include <libpsbc/psbc_compile.h>
#include "Writer/AgcBinaryWriter.hpp"

namespace AnyDevKit {

class AgcRegisterBuilder {
public:
    static void Build(const PsbcShaderMetadata& metadata, AgcBinaryWriter& writer);
};

}

#endif
