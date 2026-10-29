#ifndef ANYDEVKIT_PS5_IMPORT_CATALOG_HPP
#define ANYDEVKIT_PS5_IMPORT_CATALOG_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace AnyDevKit {

struct Ps5Import {
    std::string Nid;
    std::string Module;
    std::string FileName;
    std::string Library;
    std::uint16_t ModuleVersion;
    std::uint16_t LibraryVersion;
};

class ImportCatalog final {
public:
    static std::vector<std::string> Dependencies(std::span<const std::byte> elf);
    const std::vector<Ps5Import>& Modules() const;
    void AddModule(const std::string& fileName);
    void Select(const std::string& symbol, const std::string& module, const std::string& library);
    Ps5Import Resolve(const std::string& symbol) const;
private:
    std::vector<Ps5Import> findCandidates(const std::string& nid) const;
    std::vector<Ps5Import> modules;
    std::map<std::string, Ps5Import> selections;
};

}

#endif
