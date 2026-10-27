#ifndef ANYDEVKIT_LIBC_MODULE_BUILDER_HPP
#define ANYDEVKIT_LIBC_MODULE_BUILDER_HPP

#include "Ps5ElfBuilder.hpp"

namespace AnyDevKit {

class LibcModuleBuilder final {
public:
    static std::vector<std::byte> Build(const ImportCatalog& imports, const Ps5ElfOptions& options);
};

}

#endif
