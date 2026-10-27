#include "Reader/ElfReader.hpp"

#include <limits>
#include <stdexcept>

namespace AnyDevKit::Self {
namespace {

std::uint64_t read(std::span<const std::byte> bytes, std::size_t offset, std::size_t size) {
    if (offset > bytes.size() || size > bytes.size() - offset) throw std::invalid_argument("ELF field exceeds input");
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < size; ++index) value |= std::to_integer<std::uint64_t>(bytes[offset + index]) << (8 * index);
    return value;
}

std::span<const std::byte> slice(std::span<const std::byte> bytes, std::uint64_t offset, std::uint64_t size) {
    if (offset > bytes.size() || size > bytes.size() - offset) throw std::invalid_argument("ELF segment exceeds input");
    return bytes.subspan(static_cast<std::size_t>(offset), static_cast<std::size_t>(size));
}

}

ElfImage ElfReader::Read(std::span<const std::byte> bytes) {
    if (bytes.size() < 64 || read(bytes, 0, 4) != 0x464c457f || read(bytes, 4, 3) != 0x010102) throw std::invalid_argument("Expected little-endian ELF64 version 1");
    if (read(bytes, 18, 2) != 62 || read(bytes, 20, 4) != 1 || read(bytes, 52, 2) != 64) throw std::invalid_argument("Expected x86-64 ELF header");
    const auto type = read(bytes, 16, 2);
    if (type != 2 && type != 3 && type != 0xfe00 && type != 0xfe10) throw std::invalid_argument("Unsupported ELF executable type");
    const auto phoff = read(bytes, 32, 8);
    const auto count = read(bytes, 56, 2);
    if (count == 0 || count == 0xffff || phoff < 64 || read(bytes, 54, 2) != 56) throw std::invalid_argument("Invalid or extended ELF program header table");
    const auto table = slice(bytes, phoff, count * 56);
    const auto headerSize = static_cast<std::size_t>(phoff) + table.size();
    if (headerSize > 0xffff) throw std::length_error("ELF headers cannot fit a SELF header");
    ElfImage image{bytes, bytes.first(headerSize), {}, {}};
    const auto entry = read(bytes, 24, 8);
    bool entryMapped = false;
    bool versionFound = false;
    for (std::uint16_t index = 0; index < count; ++index) {
        const auto header = table.subspan(static_cast<std::size_t>(index) * 56, 56);
        const auto segmentType = read(header, 0, 4);
        if (segmentType == 0) continue;
        const auto flags = read(header, 4, 4);
        const auto offset = read(header, 8, 8);
        const auto address = read(header, 16, 8);
        const auto fileSize = read(header, 32, 8);
        const auto memorySize = read(header, 40, 8);
        const auto alignment = read(header, 48, 8);
        const auto data = fileSize == 0 ? std::span<const std::byte>{} : slice(bytes, offset, fileSize);
        if (alignment > 1 && (alignment & (alignment - 1)) != 0) throw std::invalid_argument("ELF segment alignment is not a power of two");
        if (memorySize > std::numeric_limits<std::uint64_t>::max() - address) throw std::invalid_argument("ELF virtual address range overflows");
        if (segmentType == 1) {
            if (offset % 0x4000 != 0 || address % 0x4000 != 0) throw std::invalid_argument("PS5 load segments must begin on 16 KiB boundaries");
            if (fileSize > memorySize || (alignment > 1 && offset % alignment != address % alignment)) throw std::invalid_argument("Invalid ELF load segment");
            if ((flags & 1) != 0 && entry >= address && entry - address < fileSize) entryMapped = true;
        }
        if (segmentType == 1 || segmentType == 0x61000000 || segmentType == 0x61000010 || segmentType == 0x6fffff00) image.Segments.push_back({index, data});
        if (segmentType == 0x6fffff01) {
            if (versionFound) throw std::invalid_argument("Duplicate ELF version segment");
            versionFound = true;
            image.Version = data;
        }
    }
    if (!entryMapped || image.Segments.empty()) throw std::invalid_argument("ELF entry point is not backed by an executable load segment");
    return image;
}

}
