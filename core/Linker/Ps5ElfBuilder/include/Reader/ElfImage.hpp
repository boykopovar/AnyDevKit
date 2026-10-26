#ifndef ANYDEVKIT_PS5_ELF_IMAGE_HPP
#define ANYDEVKIT_PS5_ELF_IMAGE_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace AnyDevKit::Ps5 {

struct Segment {
    std::uint32_t Type;
    std::uint32_t Flags;
    std::uint64_t Offset;
    std::uint64_t Address;
    std::uint64_t FileSize;
    std::uint64_t MemorySize;
    std::uint64_t Alignment;
};

struct DynamicEntry {
    std::uint64_t Tag;
    std::uint64_t Value;
};

struct Symbol {
    std::string Name;
    std::uint8_t Info;
    std::uint8_t Visibility;
    std::uint16_t Section;
    std::uint64_t Value;
    std::uint64_t Size;
};

struct Relocation {
    std::uint64_t Address;
    std::uint64_t Info;
    std::uint64_t Addend;
};

class ElfImage final {
public:
    explicit ElfImage(std::span<const std::byte> bytes);
    std::uint64_t Read(std::uint64_t offset, std::size_t size) const;
    std::span<const std::byte> Slice(std::uint64_t offset, std::uint64_t size) const;
    std::uint64_t Offset(std::uint64_t address, std::uint64_t size) const;
    std::uint64_t Dynamic(std::uint64_t tag, bool required = true) const;
    std::string String(std::uint64_t offset) const;
    std::vector<Symbol> Symbols() const;
    std::vector<Relocation> Relocations(std::uint64_t addressTag, std::uint64_t sizeTag) const;
    const std::vector<Segment>& Segments() const;
    const std::vector<DynamicEntry>& DynamicEntries() const;
private:
    std::span<const std::byte> bytes;
    std::vector<Segment> segments;
    std::vector<DynamicEntry> dynamicEntries;
    std::uint64_t tableOffset(std::uint64_t standardTag, std::uint64_t sceTag, std::uint64_t size) const;
};

}

#endif
