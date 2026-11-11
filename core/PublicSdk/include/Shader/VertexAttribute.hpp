#ifndef ANYDEVKIT_PUBLICSDK_VERTEXATTRIBUTE_HPP
#define ANYDEVKIT_PUBLICSDK_VERTEXATTRIBUTE_HPP
#include "libpsbc/psbc_compile.h"
#include <string>

namespace AnyDevKit::PublicSdk {

class VertexAttribute final {
public:
    static PsbcVertexAttribute Parse(const std::string& value);
};

}

#endif
