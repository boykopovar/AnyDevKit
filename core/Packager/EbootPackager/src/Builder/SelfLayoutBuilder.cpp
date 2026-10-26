#include "Builder/SelfLayoutBuilder.hpp"

#include <limits>
#include <stdexcept>

namespace AnyDevKit::Self {
namespace {

std::size_t add(std::size_t left, std::size_t right) {
    if (right > std::numeric_limits<std::size_t>::max() - left) throw std::length_error("SELF size overflows");
    return left + right;
}

std::size_t align(std::size_t value) {
    return add(value, 15) & ~std::size_t{15};
}

}

SelfLayout SelfLayoutBuilder::Build(const ElfImage& image) {
    if (image.Segments.empty() || image.Segments.size() > 407) throw std::length_error("SELF segment metadata exceeds 16-bit size");
    const auto entryCount = image.Segments.size() * 2;
    const auto elfOffset = add(32, entryCount * 32);
    const auto infoOffset = align(add(elfOffset, image.Header.size()));
    const auto headerSize = add(infoOffset, 64 + 48);
    const auto metadataSize = entryCount * 80 + 80 + 256;
    if (headerSize > 0xffff || metadataSize > 0xffff) throw std::length_error("SELF headers exceed 16-bit size");
    SelfLayout layout{static_cast<std::uint16_t>(headerSize), static_cast<std::uint16_t>(metadataSize), elfOffset, infoOffset, 0, 0, {}};
    auto offset = headerSize + metadataSize;
    for (const auto& segment : image.Segments) {
        const auto blocks = segment.Data.size() / 0x4000 + (segment.Data.size() % 0x4000 != 0);
        const auto digestsSize = blocks * 32;
        const auto dataIndex = layout.Entries.size() + 1;
        layout.Entries.push_back({0x10004 | (static_cast<std::uint64_t>(dataIndex) << 20), offset, digestsSize});
        offset = align(add(offset, digestsSize));
        layout.Entries.push_back({0x2804 | (static_cast<std::uint64_t>(segment.Index) << 20), offset, segment.Data.size()});
        offset = align(add(offset, segment.Data.size()));
    }
    layout.FileSize = offset;
    layout.TotalSize = add(offset, image.Version.size());
    return layout;
}

}
