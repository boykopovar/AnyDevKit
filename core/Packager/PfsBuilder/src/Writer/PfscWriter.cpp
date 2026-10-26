#include "Writer/PfscWriter.hpp"

#include <array>
#include <stdexcept>

namespace AnyDevKit::Pfs {

std::uint64_t PfscWriter::StoredSize(std::uint64_t size) {
    if (!size) throw std::invalid_argument("PFSC source is empty");
    const auto blocks = ImageWriter::Align(size, 0x10000) / 0x10000;
    const auto tableEnd = ImageWriter::Add(0x400, ImageWriter::Add(blocks, 1) * 8);
    return ImageWriter::Add(ImageWriter::Align(tableEnd, 0x10000), blocks * 0x10000);
}

void PfscWriter::Write(ImageWriter& writer, std::uint64_t offset, const std::filesystem::path& source, std::uint64_t size) {
    const auto paddedSize = ImageWriter::Align(size, 0x10000);
    const auto blocks = paddedSize / 0x10000;
    const auto headerSize = StoredSize(size) - paddedSize;
    std::array<std::byte, 0x30> header{};
    ImageWriter::Put(header, 0, 0x43534650, 4);
    ImageWriter::Put(header, 8, 6, 4);
    ImageWriter::Put(header, 12, 0x10000, 4);
    ImageWriter::Put(header, 16, 0x10000, 8);
    ImageWriter::Put(header, 24, 0x400, 8);
    ImageWriter::Put(header, 32, headerSize, 8);
    ImageWriter::Put(header, 40, paddedSize, 8);
    writer.Zero(offset, headerSize);
    writer.Write(offset, header);
    for (std::uint64_t i = 0; i <= blocks; ++i) {
        std::array<std::byte, 8> entry{};
        ImageWriter::Put(entry, 0, ImageWriter::Add(headerSize, i * 0x10000), 8);
        writer.Write(ImageWriter::Add(offset, 0x400 + i * 8), entry);
    }
    writer.Copy(ImageWriter::Add(offset, headerSize), source, size);
    writer.Zero(ImageWriter::Add(offset, ImageWriter::Add(headerSize, size)), paddedSize - size);
}

}
