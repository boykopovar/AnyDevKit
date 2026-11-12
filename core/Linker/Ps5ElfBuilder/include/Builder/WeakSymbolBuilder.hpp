#ifndef ANYDEVKIT_PS5_WEAK_SYMBOL_BUILDER_HPP
#define ANYDEVKIT_PS5_WEAK_SYMBOL_BUILDER_HPP

#include "Imports/ImportCatalog.hpp"
#include "Reader/ElfImage.hpp"
#include <map>

namespace AnyDevKit::Ps5 {

class WeakSymbolBuilder final {
public:
    static std::map<std::uint64_t, std::uint64_t> Resolve(const ImportCatalog& imports, std::vector<Symbol>& symbols, std::vector<Relocation>& relocations, std::vector<Relocation>& plt);
};

}

#endif
