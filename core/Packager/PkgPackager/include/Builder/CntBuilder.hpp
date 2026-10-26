#ifndef ANYDEVKIT_PKG_CNTBUILDER_HPP
#define ANYDEVKIT_PKG_CNTBUILDER_HPP

#include "Builder/KeyBuilder.hpp"
#include "Builder/OuterImageBuilder.hpp"

#include <string>

namespace AnyDevKit::Pkg {

struct CntEntry {
    std::uint32_t Id = 0;
    std::string Name;
    std::vector<std::byte> Data;
};

struct CntEntryInfo {
    std::uint32_t Id = 0;
    std::string Name;
    std::uint32_t Offset = 0;
    std::uint32_t Size = 0;
};

struct CntImage {
    std::vector<std::byte> Data;
    std::vector<CntEntryInfo> Entries;
};

class CntBuilder final {
public:
    static CntImage Build(std::string_view contentId, const PackageKeys& keys, const PackageKeyData& keyData, const OuterImageInfo& outer, std::span<const std::byte> fih, std::span<const CntEntry> entries);
};

}

#endif
