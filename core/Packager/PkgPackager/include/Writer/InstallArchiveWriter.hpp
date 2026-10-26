#ifndef ANYDEVKIT_PACKAGER_PKG_INSTALLARCHIVEWRITER_HPP
#define ANYDEVKIT_PACKAGER_PKG_INSTALLARCHIVEWRITER_HPP

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace AnyDevKit::Pkg {

struct InstallArchiveEntry {
    std::string Path;
    std::vector<std::byte> Content;
};

class InstallArchiveWriter final {
public:
    static std::vector<std::byte> Build(std::span<const InstallArchiveEntry> entries);
};

}

#endif
