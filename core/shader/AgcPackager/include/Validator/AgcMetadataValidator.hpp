#ifndef ANYDEVKIT_CORE_SHADER_AGCPACKAGER_INCLUDE_VALIDATOR_AGCMETADATAVALIDATOR_HPP
#define ANYDEVKIT_CORE_SHADER_AGCPACKAGER_INCLUDE_VALIDATOR_AGCMETADATAVALIDATOR_HPP

#include <libpsbc/psbc_compile.h>

namespace AnyDevKit {

class AgcMetadataValidator {
public:
    static void Validate(const PsbcShaderOutput& compiled);
};

}

#endif
