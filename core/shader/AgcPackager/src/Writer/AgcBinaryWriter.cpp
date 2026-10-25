#include "Writer/AgcBinaryWriter.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace AnyDevKit {

std::size_t AgcBinaryWriter::Reserve(std::size_t size, std::size_t alignment) {
    constexpr auto limit = std::numeric_limits<std::uint32_t>::max();
    if (alignment == 0 || (alignment & (alignment - 1)) != 0 || alignment > limit) throw std::invalid_argument("AGC: invalid block alignment");
    const auto padding = (alignment - bytes.size() % alignment) % alignment;
    if (padding > limit - bytes.size() || size > limit - bytes.size() - padding) throw std::length_error("AGC: header exceeds 32-bit size");
    const auto offset = bytes.size() + padding;
    bytes.resize(offset + size, std::byte{});
    return offset;
}

void AgcBinaryWriter::requireRange(std::size_t offset, std::size_t size) const {
    if (offset > bytes.size() || size > bytes.size() - offset) throw std::out_of_range("AGC: write outside reserved header storage");
}

void AgcBinaryWriter::writeInteger(std::size_t offset, std::uint64_t value, std::size_t size) {
    requireRange(offset, size);
    for (std::size_t index = 0; index < size; ++index) bytes[offset + index] = static_cast<std::byte>((value >> (index * 8)) & 0xffu);
}

void AgcBinaryWriter::WriteUInt8(std::size_t offset, std::uint8_t value) { writeInteger(offset, value, sizeof(value)); }
void AgcBinaryWriter::WriteUInt16(std::size_t offset, std::uint16_t value) { writeInteger(offset, value, sizeof(value)); }
void AgcBinaryWriter::WriteUInt32(std::size_t offset, std::uint32_t value) { writeInteger(offset, value, sizeof(value)); }
void AgcBinaryWriter::WriteUInt64(std::size_t offset, std::uint64_t value) { writeInteger(offset, value, sizeof(value)); }

void AgcBinaryWriter::WriteBytes(std::size_t offset, std::span<const std::byte> source) {
    requireRange(offset, source.size());
    std::copy(source.begin(), source.end(), bytes.begin() + offset);
}

void AgcBinaryWriter::WriteRelativePointer(std::size_t fieldOffset, std::size_t targetOffset) {
    requireRange(fieldOffset, sizeof(std::uint64_t));
    requireRange(targetOffset, 1);
    if (targetOffset == fieldOffset) throw std::invalid_argument("AGC: relative pointer would encode null");
    WriteUInt64(fieldOffset, static_cast<std::uint64_t>(targetOffset) - static_cast<std::uint64_t>(fieldOffset));
}

std::size_t AgcBinaryWriter::GetSize() const { return bytes.size(); }
std::vector<std::byte> AgcBinaryWriter::Finish() && { return std::move(bytes); }

}
