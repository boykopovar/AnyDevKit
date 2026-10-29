#include "Imports/ImportCatalog.hpp"
#include "Reader/ElfImage.hpp"

#include <nid/NidResolver.hpp>
#include <Ps5NidMap/ModuleNames.hpp>
#include <Ps5NidMap/ExportLibraries.hpp>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace AnyDevKit {

std::vector<std::string> ImportCatalog::Dependencies(std::span<const std::byte> elf) {
    const Ps5::ElfImage image(elf);
    std::vector<std::string> result;
    for (const auto& entry : image.DynamicEntries()) {
        if (entry.Tag != 1) continue;
        auto name = image.String(entry.Value);
        if (!name.ends_with(".prx") || name.size() <= 4 || name.find_first_of("/\\") != std::string::npos) throw std::invalid_argument("Expected an AnyPS5 .prx dependency: " + name);
        if (name == "libc.prx") name = "libSceLibcInternal.sprx";
        else name.replace(name.size() - 4, 4, ".sprx");
        if (std::ranges::find(result, name) != result.end()) throw std::invalid_argument("Duplicate mapped PS5 dependency: " + name);
        result.push_back(std::move(name));
    }
    if (result.empty()) throw std::invalid_argument("Input ELF has no DT_NEEDED dependencies");
    return result;
}

const std::vector<Ps5Import>& ImportCatalog::Modules() const { return modules; }

void ImportCatalog::AddModule(const std::string& fileName) {
    const auto& module = Ps5NidMap::GetModuleName(fileName);
    if (std::ranges::any_of(modules, [&](const auto& existing) { return existing.Module == module || existing.FileName == fileName; })) throw std::invalid_argument("Duplicate PS5 module: " + fileName);
    modules.push_back({{}, module, fileName, {}, 0, 0});
}

std::vector<Ps5Import> ImportCatalog::findCandidates(const std::string& nid) const {
    std::vector<Ps5Import> result;
    for (const auto& module : modules) {
        const std::vector<std::string>* libraries;
        try {
            libraries = &Ps5NidMap::GetExportLibraries(module.Module, nid);
        } catch (const std::out_of_range&) {
            continue;
        }
        for (const auto& library : *libraries) result.push_back({nid, module.Module, module.FileName, library, 0, 0});
    }
    return result;
}

Ps5Import ImportCatalog::Resolve(const std::string& symbol) const {
    const auto nid = Nid::ResolveOneName(symbol);
    const auto selected = selections.find(nid);
    if (selected != selections.end()) return selected->second;
    const auto candidates = findCandidates(nid);
    if (candidates.empty()) throw std::invalid_argument("No PS5 export for " + symbol + " (" + nid + ")");
    if (candidates.size() != 1) throw std::invalid_argument("Select a module and library for ambiguous PS5 import: " + symbol);
    return candidates.front();
}

void ImportCatalog::Select(const std::string& symbol, const std::string& module, const std::string& library) {
    const auto nid = Nid::ResolveOneName(symbol);
    const auto candidates = findCandidates(nid);
    const auto candidate = std::ranges::find_if(candidates, [&](const auto& imported) { return imported.Module == module && imported.Library == library; });
    if (candidate == candidates.end()) throw std::invalid_argument("Selected PS5 library does not export " + symbol);
    selections.insert_or_assign(nid, *candidate);
}

}
