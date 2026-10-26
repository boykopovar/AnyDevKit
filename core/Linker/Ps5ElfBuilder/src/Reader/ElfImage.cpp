#include "Reader/ElfImage.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace AnyDevKit::Ps5 {

ElfImage::ElfImage(std::span<const std::byte> bytes) : bytes(bytes) {
    if (Read(0, 4) != 0x464c457f || Read(4, 3) != 0x010102 || Read(18, 2) != 62 || Read(20, 4) != 1 || Read(52, 2) != 64 || Read(54, 2) != 56) throw std::invalid_argument("Expected little-endian x86-64 ELF64");
    const auto phoff = Read(32, 8);
    const auto count = Read(56, 2);
    if (count == 0 || count == 0xffff || phoff < 64) throw std::invalid_argument("Unsupported ELF program headers");
    Slice(phoff, count * 56);
    for (std::uint64_t i = 0; i < count; ++i) {
        const auto offset = phoff + i * 56;
        Segment segment{static_cast<std::uint32_t>(Read(offset, 4)), static_cast<std::uint32_t>(Read(offset + 4, 4)), Read(offset + 8, 8), Read(offset + 16, 8), Read(offset + 32, 8), Read(offset + 40, 8), Read(offset + 48, 8)};
        if (segment.FileSize != 0 && (segment.Type == 1 || segment.Type == 2 || segment.Type == 0x61000000)) Slice(segment.Offset, segment.FileSize);
        if (segment.MemorySize > std::numeric_limits<std::uint64_t>::max() - segment.Address) throw std::invalid_argument("ELF segment address overflows");
        if (segment.Alignment > 1 && (segment.Alignment & (segment.Alignment - 1)) != 0) throw std::invalid_argument("ELF alignment is not a power of two");
        if (segment.Type == 1 && (segment.FileSize > segment.MemorySize || (segment.Alignment > 1 && segment.Offset % segment.Alignment != segment.Address % segment.Alignment))) throw std::invalid_argument("Invalid ELF load segment");
        segments.push_back(segment);
    }
    const Segment* dynamic = nullptr;
    for (const auto& segment : segments) {
        if (segment.Type != 2) continue;
        if (dynamic != nullptr) throw std::invalid_argument("Multiple ELF dynamic segments");
        dynamic = &segment;
    }
    if (dynamic == nullptr || dynamic->FileSize % 16 != 0) throw std::invalid_argument("Missing or malformed ELF dynamic table");
    for (std::uint64_t offset = 0; offset < dynamic->FileSize; offset += 16) {
        const auto tag = Read(dynamic->Offset + offset, 8);
        if (tag == 0) return;
        dynamicEntries.push_back({tag, Read(dynamic->Offset + offset + 8, 8)});
    }
    throw std::invalid_argument("Unterminated ELF dynamic table");
}

std::span<const std::byte> ElfImage::Slice(std::uint64_t offset, std::uint64_t size) const {
    if (offset > bytes.size() || size > bytes.size() - offset) throw std::invalid_argument("ELF range exceeds input");
    return bytes.subspan(static_cast<std::size_t>(offset), static_cast<std::size_t>(size));
}

std::uint64_t ElfImage::Read(std::uint64_t offset, std::size_t size) const {
    if (size > 8) throw std::invalid_argument("ELF integer exceeds 64 bits");
    const auto data = Slice(offset, size);
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < size; ++i) value |= std::to_integer<std::uint64_t>(data[i]) << (i * 8);
    return value;
}

std::uint64_t ElfImage::Offset(std::uint64_t address, std::uint64_t size) const {
    for (const auto& segment : segments) {
        if (segment.Type == 1 && address >= segment.Address && address - segment.Address <= segment.FileSize && size <= segment.FileSize - (address - segment.Address)) return segment.Offset + address - segment.Address;
    }
    throw std::invalid_argument("ELF address is not file-backed");
}

std::uint64_t ElfImage::Dynamic(std::uint64_t tag, bool required) const {
    const DynamicEntry* found = nullptr;
    for (const auto& entry : dynamicEntries) {
        if (entry.Tag != tag) continue;
        if (found != nullptr) throw std::invalid_argument("Duplicate ELF dynamic tag");
        found = &entry;
    }
    if (found == nullptr && required) throw std::invalid_argument("Missing ELF dynamic tag: " + std::to_string(tag));
    return found == nullptr ? 0 : found->Value;
}

std::uint64_t ElfImage::tableOffset(std::uint64_t standardTag, std::uint64_t sceTag, std::uint64_t size) const {
    const bool standard = std::ranges::any_of(dynamicEntries, [&](const auto& entry) { return entry.Tag == standardTag; });
    if (standard) return Offset(Dynamic(standardTag), size);
    const auto relative = Dynamic(sceTag);
    const Segment* found = nullptr;
    for (const auto& segment : segments) {
        if (segment.Type != 0x61000000) continue;
        if (found != nullptr) throw std::invalid_argument("Multiple SCE dynamic data segments");
        found = &segment;
    }
    if (found == nullptr || relative > found->FileSize || size > found->FileSize - relative) throw std::invalid_argument("Invalid SCE dynamic data range");
    return found->Offset + relative;
}

std::string ElfImage::String(std::uint64_t offset) const {
    const bool standard = std::ranges::any_of(dynamicEntries, [](const auto& entry) { return entry.Tag == 10; });
    const auto size = Dynamic(standard ? 10 : 0x61000037);
    if (offset >= size) throw std::invalid_argument("ELF string offset exceeds table");
    const auto data = Slice(tableOffset(5, 0x61000035, size) + offset, size - offset);
    const auto end = std::ranges::find(data, std::byte{0});
    if (end == data.end()) throw std::invalid_argument("Unterminated ELF string");
    return std::string(reinterpret_cast<const char*>(data.data()), static_cast<std::size_t>(end - data.begin()));
}

std::vector<Symbol> ElfImage::Symbols() const {
    std::uint64_t size = Dynamic(0x6100003f, false);
    if (size == 0) {
        const auto hash = tableOffset(4, 0x61000025, 8);
        size = Read(hash + 4, 4) * 24;
    }
    if (size < 24 || size % 24 != 0) throw std::invalid_argument("Malformed ELF symbol table");
    const auto offset = tableOffset(6, 0x61000039, size);
    std::vector<Symbol> result;
    for (std::uint64_t i = 0; i < size; i += 24) result.push_back({String(Read(offset + i, 4)), static_cast<std::uint8_t>(Read(offset + i + 4, 1)), static_cast<std::uint8_t>(Read(offset + i + 5, 1)), static_cast<std::uint16_t>(Read(offset + i + 6, 2)), Read(offset + i + 8, 8), Read(offset + i + 16, 8)});
    return result;
}

std::vector<Relocation> ElfImage::Relocations(std::uint64_t addressTag, std::uint64_t sizeTag) const {
    const auto size = Dynamic(sizeTag, false);
    if (size == 0) return {};
    if (size % 24 != 0 || (addressTag == 7 && Dynamic(9) != 24) || (addressTag == 23 && Dynamic(20) != 7)) throw std::invalid_argument("Expected ELF RELA relocations");
    const auto offset = Offset(Dynamic(addressTag), size);
    std::vector<Relocation> result;
    for (std::uint64_t i = 0; i < size; i += 24) result.push_back({Read(offset + i, 8), Read(offset + i + 8, 8), Read(offset + i + 16, 8)});
    return result;
}

const std::vector<Segment>& ElfImage::Segments() const { return segments; }
const std::vector<DynamicEntry>& ElfImage::DynamicEntries() const { return dynamicEntries; }

}
