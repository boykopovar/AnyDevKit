#ifndef ANYDEVKIT_PUBLICSDK_SHADERMETADATA_HPP
#define ANYDEVKIT_PUBLICSDK_SHADERMETADATA_HPP
#include <libpsbc/psbc_compile.h>
#include <string>
#include <string_view>
namespace AnyDevKit::PublicSdk {
class ShaderMetadata final {
public:
    static std::string Encode(const PsbcShaderMetadata& metadata);
    static PsbcShaderMetadata Decode(std::string_view text);
};
}
#endif
