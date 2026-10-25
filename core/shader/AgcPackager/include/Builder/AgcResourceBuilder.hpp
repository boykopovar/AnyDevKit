#pragma once

#include <libpsbc/psbc_compile.h>
#include "Writer/AgcBinaryWriter.hpp"

namespace AnyDevKit {

class AgcResourceBuilder {
public:
    static void Build(const PsbcShaderMetadata& metadata, AgcBinaryWriter& writer);
};

}
