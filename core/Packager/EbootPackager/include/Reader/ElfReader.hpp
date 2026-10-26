#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace AnyDevKit::Self {

struct ElfSegment {
    std::uint16_t Index;
    std::span<const std::byte> Data;
};

struct ElfImage {
    std::span<const std::byte> Source;
    std::span<const std::byte> Header;
    std::vector<ElfSegment> Segments;
    std::span<const std::byte> Version;
};

class ElfReader final {
public:
    static ElfImage Read(std::span<const std::byte> bytes);
};

}
