#include "WindowsStartupBuilder.hpp"
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace AnyDevKit {
void WindowsStartupBuilder::PrepareExecutable(std::span<std::byte> image) {
    const auto read = [&](std::size_t offset, std::size_t size) {
        if (offset > image.size() || size > image.size() - offset) throw std::invalid_argument("Truncated PE executable");
        std::uint32_t value = 0;
        for (std::size_t i = 0; i < size; ++i) value |= std::to_integer<std::uint32_t>(image[offset + i]) << (8 * i);
        return value;
    };
    if (read(0, 2) != 0x5a4d) throw std::invalid_argument("Expected PE executable");
    const std::size_t pe = read(0x3c, 4);
    if (read(pe, 4) != 0x4550 || read(pe + 4, 2) != 0x8664 || read(pe + 24, 2) != 0x20b) throw std::invalid_argument("Expected x86-64 PE executable");
    const auto count = read(pe + 6, 2);
    const auto optionalSize = read(pe + 20, 2);
    if (optionalSize < 112) throw std::invalid_argument("Truncated PE optional header");
    const auto sections = pe + 24 + optionalSize;
    if (sections > image.size() || count > (image.size() - sections) / 40) throw std::invalid_argument("Truncated PE section table");
    bool unwind = false;
    for (std::size_t i = 0; i < count; ++i) {
        const auto offset = sections + i * 40;
        const std::string_view name(reinterpret_cast<const char*>(image.data() + offset), 8);
        std::string_view replacement;
        if (name == ".eh_fram") { replacement = ".ehfram"; unwind = true; }
        if (name == std::string_view(".rdata\0\0", 8)) {
            replacement = ".sdkdata";
            image[offset + 39] |= std::byte{0x80};
        }
        if (!replacement.empty()) {
            std::fill_n(image.begin() + offset, 8, std::byte{});
            for (std::size_t j = 0; j < replacement.size(); ++j) image[offset + j] = std::byte(replacement[j]);
        }
    }
    if (!unwind) throw std::invalid_argument("Missing DWARF exception section");
}
}
