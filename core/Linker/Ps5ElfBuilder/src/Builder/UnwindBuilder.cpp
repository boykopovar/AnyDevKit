#include "Builder/UnwindBuilder.hpp"
#include "Writer/BinaryWriter.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace AnyDevKit::Ps5 {
namespace {

void relative(std::vector<std::byte>& bytes, std::uint64_t value, std::uint64_t base) {
    if ((value >= base && value - base > 0x7fffffff) || (value < base && base - value > 0x80000000)) throw std::invalid_argument("Unwind address exceeds signed 32-bit range");
    BinaryWriter::Append(bytes, static_cast<std::uint32_t>(value - base), 4);
}

std::uint64_t encoded(const ElfImage& image, std::uint64_t& cursor, std::uint8_t encoding, std::uint64_t dataBase) {
    const auto format = encoding & 15;
    const auto application = encoding & 0x70;
    if ((encoding & 0x80) != 0 || (format != 0 && format != 3 && format != 4 && format != 11 && format != 12) || (application != 0 && application != 0x10 && application != 0x30)) throw std::invalid_argument("Unsupported unwind header encoding");
    const std::size_t width = format == 3 || format == 11 ? 4 : 8;
    auto value = image.Read(image.Offset(cursor, width), width);
    if (format == 11) value = static_cast<std::uint64_t>(static_cast<std::int64_t>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value))));
    const auto base = application == 0x10 ? cursor : application == 0x30 ? dataBase : 0;
    cursor = BinaryWriter::Add(cursor, width);
    if ((format == 11 || format == 12) && (value >> 63) != 0) {
        const auto magnitude = ~value + 1;
        if (magnitude > base) throw std::invalid_argument("Unwind address underflows");
        return base - magnitude;
    }
    return BinaryWriter::Add(base, value);
}

}

UnwindImage UnwindBuilder::Build(const ElfImage& image, const StartupImage& startup, std::uint64_t codeAddress, std::uint64_t address) {
    std::vector<std::pair<std::uint64_t, std::uint64_t>> entries;
    std::uint64_t originalFrames = 0;
    for (const auto& segment : image.Segments()) {
        if (segment.Type != 0x6474e550) continue;
        if (originalFrames != 0 || segment.FileSize < 4 || image.Read(segment.Offset, 1) != 1) throw std::invalid_argument("Invalid ELF unwind header");
        auto cursor = segment.Address + 4;
        originalFrames = encoded(image, cursor, static_cast<std::uint8_t>(image.Read(segment.Offset + 1, 1)), segment.Address);
        const auto count = encoded(image, cursor, static_cast<std::uint8_t>(image.Read(segment.Offset + 2, 1)), segment.Address);
        const auto tableEncoding = static_cast<std::uint8_t>(image.Read(segment.Offset + 3, 1));
        if (count > segment.FileSize / 8) throw std::invalid_argument("Unwind table exceeds segment");
        for (std::uint64_t i = 0; i < count; ++i) {
            const auto start = encoded(image, cursor, tableEncoding, segment.Address);
            const auto fde = encoded(image, cursor, tableEncoding, segment.Address);
            image.Offset(start, 1);
            image.Offset(fde, 4);
            entries.emplace_back(start, fde);
        }
        if (cursor > segment.Address + segment.FileSize) throw std::invalid_argument("Unwind table exceeds segment");
    }
    UnwindImage result{{}, 0, 0};
    auto byte = [&](unsigned value) { result.Bytes.push_back(static_cast<std::byte>(value)); };
    BinaryWriter::Append(result.Bytes, 20, 4);
    BinaryWriter::Append(result.Bytes, 0, 4);
    for (unsigned value : {1u, 0x7au, 0x52u, 0u, 1u, 0x78u, 16u, 1u, 0x1bu, 0x0cu, 7u, 8u, 0x90u, 1u, 0u, 0u}) byte(value);
    auto fde = [&](std::uint64_t start, std::uint64_t size, std::initializer_list<unsigned> instructions) {
        const auto offset = result.Bytes.size();
        entries.emplace_back(start, address + offset);
        BinaryWriter::Append(result.Bytes, 0, 4);
        BinaryWriter::Append(result.Bytes, offset + 4, 4);
        relative(result.Bytes, start, address + result.Bytes.size());
        BinaryWriter::Append(result.Bytes, size, 4);
        byte(0);
        for (auto value : instructions) byte(value);
        while (result.Bytes.size() % 8 != 0) byte(0);
        BinaryWriter::Set(result.Bytes, offset, result.Bytes.size() - offset - 4, 4);
    };
    fde(codeAddress, startup.Entry - codeAddress, {0x44, 0x0e, 16});
    fde(startup.Entry, startup.Bytes.size() - (startup.Entry - codeAddress), {0x41, 0x0e, 16, 0x86, 2, 0x43, 0x0d, 6, 0x42, 0x8c, 3, 0x42, 0x8d, 4, 0x42, 0x8e, 5});
    BinaryWriter::Append(result.Bytes, 0, 4);
    result.HeaderOffset = result.Bytes.size();
    const auto headerAddress = address + result.HeaderOffset;
    BinaryWriter::Append(result.Bytes, 0x3b031b01, 4);
    relative(result.Bytes, originalFrames == 0 ? address : originalFrames, headerAddress + 4);
    if (entries.size() > 0xffffffff) throw std::length_error("Too many unwind entries");
    BinaryWriter::Append(result.Bytes, entries.size(), 4);
    std::ranges::sort(entries);
    for (const auto& [start, fdeAddress] : entries) {
        relative(result.Bytes, start, headerAddress);
        relative(result.Bytes, fdeAddress, headerAddress);
    }
    result.HeaderSize = result.Bytes.size() - result.HeaderOffset;
    return result;
}

}
