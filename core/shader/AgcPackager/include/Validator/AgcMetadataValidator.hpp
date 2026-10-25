#pragma once

#include <libpsbc/psbc_compile.h>

namespace AnyDevKit {

class AgcMetadataValidator {
public:
    static void Validate(const PsbcShaderOutput& compiled);
};

}
