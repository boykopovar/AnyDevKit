#ifndef ANYDEVKIT_PACKAGER_PFSBUILDER_HPP
#define ANYDEVKIT_PACKAGER_PFSBUILDER_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace AnyDevKit {

struct PfsEntry {
    std::string Path;
    std::filesystem::path Source;
    bool Directory = false;
    bool Pfsc = false;
};

struct PfsOptions {
    std::uint32_t BlockSize = 0x10000;
    std::int64_t Timestamp = 0;
    bool CaseInsensitive = false;
};

class PfsBuilder final {
public:
    static std::vector<PfsEntry> ScanDirectory(const std::filesystem::path& input);
    static void Build(const std::vector<PfsEntry>& entries, const std::filesystem::path& output, const PfsOptions& options = {});
};

}

#endif
