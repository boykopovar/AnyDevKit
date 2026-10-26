#include "Writer/BinaryWriter.hpp"

#include <limits>
#include <stdexcept>

namespace AnyDevKit::Ps5 {

std::uint64_t BinaryWriter::Add(std::uint64_t left, std::uint64_t right) {
    if (right > std::numeric_limits<std::uint64_t>::max() - left) throw std::length_error("ELF size overflows");
    return left + right;
}

std::uint64_t BinaryWriter::Align(std::uint64_t value, std::uint64_t alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) throw std::invalid_argument("Invalid ELF alignment");
    return Add(value, alignment - 1) & ~(alignment - 1);
}

void BinaryWriter::Set(std::span<std::byte> output, std::uint64_t offset, std::uint64_t value, std::size_t size) {
    if (size > 8 || offset > output.size() || size > output.size() - offset) throw std::out_of_range("ELF write exceeds output");
    for (std::size_t i = 0; i < size; ++i) output[static_cast<std::size_t>(offset) + i] = static_cast<std::byte>((value >> (i * 8)) & 255);
}

void BinaryWriter::Append(std::vector<std::byte>& output, std::uint64_t value, std::size_t size) {
    const auto offset = output.size();
    if (size > output.max_size() - offset) throw std::length_error("ELF buffer exceeds maximum size");
    output.resize(offset + size);
    Set(output, offset, value, size);
}

std::uint32_t BinaryWriter::String(std::vector<std::byte>& output, std::string_view value) {
    if (output.size() > 0xffffffff || value.size() > 0xffffffff - output.size() || value.find('\0') != std::string_view::npos) throw std::invalid_argument("Invalid ELF string table entry");
    const auto offset = static_cast<std::uint32_t>(output.size());
    for (char c : value) output.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
    output.push_back(std::byte{0});
    return offset;
}

}
