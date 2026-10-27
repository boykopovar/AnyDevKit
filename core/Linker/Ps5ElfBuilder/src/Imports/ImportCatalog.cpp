#include "Imports/ImportCatalog.hpp"
#include "Reader/ElfImage.hpp"

#include <nid/NidResolver.hpp>
#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace AnyDevKit {
namespace {

std::uint16_t decode(std::string_view value) {
    constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
    if (value.empty() || value.size() > 3) throw std::invalid_argument("Invalid SCE symbol identifier");
    std::uint32_t result = 0;
    for (char c : value) {
        const auto digit = alphabet.find(c);
        if (digit == std::string_view::npos) throw std::invalid_argument("Invalid SCE symbol identifier");
        result = (result << 6) | static_cast<std::uint32_t>(digit);
    }
    if (result > 0xffff) throw std::invalid_argument("SCE symbol identifier exceeds 16 bits");
    return static_cast<std::uint16_t>(result);
}

}

void ImportCatalog::AddModule(std::span<const std::byte> elf, const std::string& fileName) {
    if (fileName.empty() || fileName.find_first_of("/\\\0", 0, 3) != std::string::npos || (!fileName.ends_with(".prx") && !fileName.ends_with(".sprx"))) throw std::invalid_argument("Expected a PS5 module filename ending in .prx or .sprx");
    const Ps5::ElfImage image(elf);
    if (image.Read(16, 2) != 0xfe18) throw std::invalid_argument("Import catalog requires a decrypted PS5 dynamic module");
    const auto info = image.Dynamic(0x61000043);
    const auto moduleId = static_cast<std::uint16_t>(info >> 48);
    const auto module = image.String(info & 0xffffffff);
    std::map<std::uint16_t, std::pair<std::string, std::uint16_t>> libraries;
    for (const auto& entry : image.DynamicEntries()) {
        if (entry.Tag == 0x61000047 && !libraries.emplace(static_cast<std::uint16_t>(entry.Value >> 48), std::pair{image.String(entry.Value & 0xffffffff), static_cast<std::uint16_t>(entry.Value >> 32)}).second) throw std::invalid_argument("Duplicate SCE export library");
    }
    auto updated = imports;
    for (const auto& symbol : image.Symbols()) {
        if (symbol.Section == 0 || (symbol.Info >> 4) == 0 || (symbol.Visibility & 3) == 2) continue;
        const auto separator = symbol.Name.find('#');
        if (separator == std::string::npos) continue;
        const auto second = symbol.Name.find('#', separator + 1);
        if (separator != 11 || second == std::string::npos || decode(std::string_view(symbol.Name).substr(second + 1)) != moduleId) throw std::invalid_argument("Malformed SCE exported symbol");
        const auto libraryId = decode(std::string_view(symbol.Name).substr(separator + 1, second - separator - 1));
        const auto library = libraries.find(libraryId);
        if (library == libraries.end()) throw std::invalid_argument("Export refers to an unknown SCE library");
        Ps5Import imported{symbol.Name.substr(0, separator), module, fileName, library->second.first, static_cast<std::uint16_t>(info >> 32), library->second.second, static_cast<std::uint8_t>(symbol.Info & 15)};
        auto& candidates = updated[imported.Nid];
        const auto existing = std::ranges::find_if(candidates, [&](const auto& candidate) { return candidate.Module == imported.Module && candidate.Library == imported.Library; });
        if (existing != candidates.end()) {
            if (existing->Type != imported.Type || existing->FileName != imported.FileName || existing->ModuleVersion != imported.ModuleVersion || existing->LibraryVersion != imported.LibraryVersion) throw std::invalid_argument("Conflicting PS5 export: " + imported.Nid);
        } else candidates.push_back(std::move(imported));
    }
    imports = std::move(updated);
}

Ps5Import ImportCatalog::Resolve(const std::string& symbol) const {
    const auto nid = Nid::ResolveOneName(symbol);
    const auto selected = selections.find(nid);
    if (selected != selections.end()) return selected->second;
    const auto found = imports.find(nid);
    if (found == imports.end()) throw std::invalid_argument("No PS5 export for " + symbol + " (" + nid + ")");
    if (found->second.size() != 1) throw std::invalid_argument("Select a module and library for ambiguous PS5 import: " + symbol);
    return found->second.front();
}

void ImportCatalog::Select(const std::string& symbol, const std::string& module, const std::string& library) {
    const auto nid = Nid::ResolveOneName(symbol);
    const auto found = imports.find(nid);
    if (found == imports.end()) throw std::invalid_argument("No PS5 export for " + symbol);
    const auto candidate = std::ranges::find_if(found->second, [&](const auto& imported) { return imported.Module == module && imported.Library == library; });
    if (candidate == found->second.end()) throw std::invalid_argument("Selected PS5 library does not export " + symbol);
    selections.insert_or_assign(nid, *candidate);
}

}
