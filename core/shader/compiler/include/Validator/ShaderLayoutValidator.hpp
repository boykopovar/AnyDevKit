#pragma once

#include <glslang/Public/ShaderLang.h>
#include <libpsbc/psbc_compile.h>

namespace AnyDevKit {

class ShaderLayoutValidator {
public:
    static void Validate(glslang::TProgram& program, const PsbcCompileOptions& options);
};

}
