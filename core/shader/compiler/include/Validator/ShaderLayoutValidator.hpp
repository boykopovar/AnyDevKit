#ifndef ANYDEVKIT_CORE_SHADER_COMPILER_INCLUDE_VALIDATOR_SHADERLAYOUTVALIDATOR_HPP
#define ANYDEVKIT_CORE_SHADER_COMPILER_INCLUDE_VALIDATOR_SHADERLAYOUTVALIDATOR_HPP

#include <glslang/Public/ShaderLang.h>
#include <libpsbc/psbc_compile.h>

namespace AnyDevKit {

class ShaderLayoutValidator {
public:
    static void Validate(glslang::TProgram& program, const PsbcCompileOptions& options);
};

}

#endif
