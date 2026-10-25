#pragma once

#include <libpsbc/psbc_compile.h>
#include "Writer/AgcBinaryWriter.hpp"

namespace AnyDevKit {

class AgcSemanticBuilder {
public:
    static void Build(const PsbcShaderMetadata& metadata, AgcBinaryWriter& writer);
};

}
