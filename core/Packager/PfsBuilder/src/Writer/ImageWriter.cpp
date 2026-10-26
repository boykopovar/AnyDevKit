#include "Writer/ImageWriter.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace AnyDevKit::Pfs {

ImageWriter::ImageWriter(const std::filesystem::path& output) {
    stream.exceptions(std::ios::badbit | std::ios::failbit);
    stream.open(output, std::ios::binary | std::ios::trunc);
}

std::uint64_t ImageWriter::Add(std::uint64_t left, std::uint64_t right) {
    if (right > std::numeric_limits<std::uint64_t>::max() - left) throw std::overflow_error("Image size overflow");
    return left + right;
}

std::uint64_t ImageWriter::Align(std::uint64_t value, std::uint64_t alignment) {
    if (!alignment) throw std::invalid_argument("Zero image alignment");
    return Add(value, (alignment - value % alignment) % alignment);
}

void ImageWriter::Put(std::span<std::byte> bytes, std::size_t offset, std::uint64_t value, std::size_t width) {
    if (width > 8 || offset > bytes.size() || width > bytes.size() - offset) throw std::out_of_range("Image field exceeds buffer");
    if (width < 8 && (value >> (width * 8)) != 0) throw std::overflow_error("Image field overflow");
    for (std::size_t i = 0; i < width; ++i) bytes[offset + i] = static_cast<std::byte>((value >> (i * 8)) & 0xff);
}

void ImageWriter::Write(std::uint64_t offset, std::span<const std::byte> bytes) {
    if (Add(offset, bytes.size()) > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) throw std::length_error("Image exceeds stream range");
    if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) throw std::length_error("Write exceeds stream range");
    stream.seekp(static_cast<std::streamoff>(offset));
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

void ImageWriter::Copy(std::uint64_t offset, const std::filesystem::path& input, std::uint64_t size) {
    if (std::filesystem::file_size(input) != size) throw std::runtime_error("PFS source size changed");
    std::ifstream source;
    source.exceptions(std::ios::badbit | std::ios::failbit);
    source.open(input, std::ios::binary);
    std::array<std::byte, 0x10000> buffer{};
    for (std::uint64_t position = 0; position < size;) {
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(buffer.size(), size - position));
        source.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(count));
        Write(Add(offset, position), std::span(buffer).first(count));
        position += count;
    }
    if (source.rdbuf()->sgetc() != std::char_traits<char>::eof()) throw std::runtime_error("PFS source grew while reading");
    source.close();
}

void ImageWriter::Zero(std::uint64_t offset, std::uint64_t size) {
    const std::array<std::byte, 0x10000> buffer{};
    for (std::uint64_t position = 0; position < size;) {
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(buffer.size(), size - position));
        Write(Add(offset, position), std::span(buffer).first(count));
        position += count;
    }
}

void ImageWriter::Close() {
    stream.close();
}

}
