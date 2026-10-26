#pragma once

#include "Reader/ElfReader.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace AnyDevKit::Self {

struct SelfEntry {
    std::uint64_t Properties;
    std::size_t Offset;
    std::size_t Size;
};

struct SelfLayout {
    std::uint16_t HeaderSize;
    std::uint16_t MetadataSize;
    std::size_t ElfOffset;
    std::size_t InfoOffset;
    std::size_t FileSize;
    std::size_t TotalSize;
    std::vector<SelfEntry> Entries;
};

class SelfLayoutBuilder final {
public:
    static SelfLayout Build(const ElfImage& image);
};

}
