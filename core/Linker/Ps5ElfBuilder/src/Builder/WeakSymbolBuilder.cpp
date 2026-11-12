#include "Builder/WeakSymbolBuilder.hpp"
#include <limits>
#include <stdexcept>
#include <utility>

namespace AnyDevKit::Ps5 {

std::map<std::uint64_t, std::uint64_t> WeakSymbolBuilder::Resolve(const ImportCatalog& imports, std::vector<Symbol>& symbols, std::vector<Relocation>& relocations, std::vector<Relocation>& plt) {
    if (symbols.empty()) throw std::invalid_argument("Missing ELF null symbol");
    std::vector<bool> unresolved(symbols.size());
    std::vector<std::uint32_t> indices(symbols.size());
    std::vector<Symbol> retained;
    for (std::size_t i = 0; i < symbols.size(); ++i) {
        const auto& symbol = symbols[i];
        if (i != 0 && symbol.Section == 0 && (symbol.Info >> 4) == 2 && ((symbol.Visibility & 3) != 0 || !imports.Find(symbol.Name))) {
            if ((symbol.Info & 15) > 2 || symbol.Value != 0) throw std::invalid_argument("Unsupported unresolved weak symbol: " + symbol.Name);
            unresolved[i] = true;
        } else {
            if (retained.size() > std::numeric_limits<std::uint32_t>::max()) throw std::length_error("ELF symbol index exceeds 32 bits");
            indices[i] = static_cast<std::uint32_t>(retained.size());
            retained.push_back(symbol);
        }
    }

    std::map<std::uint64_t, std::uint64_t> writes;
    auto rewrite = [&](std::vector<Relocation>& entries) {
        std::vector<Relocation> kept;
        for (auto relocation : entries) {
            const auto index = relocation.Info >> 32;
            const auto type = static_cast<std::uint32_t>(relocation.Info);
            if (index >= symbols.size()) throw std::invalid_argument("ELF relocation symbol exceeds table");
            if (unresolved[index]) {
                if (type != 1 && type != 6 && type != 7) throw std::invalid_argument("Unsupported unresolved weak relocation");
                if (!writes.emplace(relocation.Address, type == 1 ? relocation.Addend : 0).second) throw std::invalid_argument("Duplicate unresolved weak relocation target");
            } else {
                relocation.Info = (static_cast<std::uint64_t>(indices[index]) << 32) | type;
                kept.push_back(relocation);
            }
        }
        entries = std::move(kept);
    };
    rewrite(relocations);
    rewrite(plt);
    symbols = std::move(retained);
    return writes;
}

}
