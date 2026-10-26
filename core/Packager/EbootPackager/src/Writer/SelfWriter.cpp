#include "Writer/SelfWriter.hpp"
#include "Sha256.hpp"

#include <algorithm>
#include <stdexcept>

namespace AnyDevKit::Self {
namespace {

void write(std::span<std::byte> output, std::size_t offset, std::uint64_t value, std::size_t size) {
    if (offset > output.size() || size > output.size() - offset) throw std::out_of_range("SELF write exceeds output");
    for (std::size_t i = 0; i < size; ++i) output[offset + i] = static_cast<std::byte>((value >> (i * 8)) & 0xff);
}

void copy(std::span<std::byte> output, std::size_t offset, std::span<const std::byte> input) {
    if (offset > output.size() || input.size() > output.size() - offset) throw std::out_of_range("SELF payload exceeds output");
    std::ranges::copy(input, output.subspan(offset).begin());
}

}

std::vector<std::byte> SelfWriter::Write(const ElfImage& image, const EbootPackageOptions& options) {
    const auto layout = SelfLayoutBuilder::Build(image);
    std::vector<std::byte> output(layout.TotalSize);
    write(output, 0, 0x1d3d154f, 4);
    write(output, 4, 0x12010100, 4);
    write(output, 8, 0x101, 4);
    write(output, 12, layout.HeaderSize, 2);
    write(output, 14, layout.MetadataSize, 2);
    write(output, 16, layout.FileSize, 8);
    write(output, 24, layout.Entries.size(), 2);
    write(output, 26, 0x22, 2);
    for (std::size_t i = 0; i < layout.Entries.size(); ++i) {
        const auto& entry = layout.Entries[i];
        const auto offset = 32 + i * 32;
        write(output, offset, entry.Properties, 8);
        write(output, offset + 8, entry.Offset, 8);
        write(output, offset + 16, entry.Size, 8);
        write(output, offset + 24, entry.Size, 8);
    }
    copy(output, layout.ElfOffset, image.Header);
    write(output, layout.ElfOffset + 60, 0, 2);
    write(output, layout.InfoOffset, options.ProgramAuthorityId, 8);
    write(output, layout.InfoOffset + 8, 1, 8);
    write(output, layout.InfoOffset + 16, options.ApplicationVersion, 8);
    write(output, layout.InfoOffset + 24, options.FirmwareVersion, 8);
    copy(output, layout.InfoOffset + 32, Crypto::Sha256::Compute(image.Source));
    write(output, layout.InfoOffset + 64, 3, 2);
    write(output, layout.HeaderSize + layout.Entries.size() * 80 + 48, 0x10000, 4);
    for (std::size_t i = 0; i < image.Segments.size(); ++i) copy(output, layout.Entries[i * 2 + 1].Offset, image.Segments[i].Data);
    copy(output, layout.FileSize, image.Version);
    return output;
}

}
