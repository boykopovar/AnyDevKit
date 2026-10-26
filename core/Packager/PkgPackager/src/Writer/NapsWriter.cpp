#include "Writer/NapsWriter.hpp"
#include "Writer/ImageWriter.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace AnyDevKit::Pkg {

std::vector<std::byte> NapsWriter::Build(const InnerImageInfo& image) {
    using Pfs::ImageWriter;
    constexpr std::uint64_t unit = 0x40000;
    if (!image.Size || image.Size % 0x10000 || image.MetadataOffset % unit || image.MetadataOffset >= image.Size || image.Files.empty()) throw std::invalid_argument("Invalid raw NAPS image geometry");
    const auto outerBlocks = image.Size / 0x10000;
    const auto units = (image.Size + unit - 1) / unit;
    const auto recordCount = 2 * (units + 1);
    const auto fileCount = image.Files.size() + 2;
    if (outerBlocks > 0xffffff || units > 0x400000 || recordCount - 2 > 0xffffff || fileCount > 0x800000) throw std::length_error("NAPS image exceeds packed field ranges");
    std::uint64_t previous = 0;
    for (std::size_t i = 0; i < image.Files.size(); ++i) {
        const auto& file = image.Files[i];
        if (file.Offset % unit || (i == 0 && file.Offset) || (i && file.Offset <= previous) || ImageWriter::Add(file.Offset, file.Size) > image.MetadataOffset) throw std::invalid_argument("Invalid raw NAPS file placement");
        previous = file.Offset;
    }
    const auto fileStart = 16 + outerBlocks * 8;
    const auto indexStart = fileStart + fileCount * 6;
    const auto groups = (units + 7) / 8;
    const auto recordStart = ImageWriter::Align(indexStart + groups * 10, 8);
    std::vector<std::byte> result(static_cast<std::size_t>(ImageWriter::Add(ImageWriter::Align(recordStart + recordCount * 9, 8), 8)));
    ImageWriter::Put(result, 0, (fileCount - 1) | (std::uint64_t{2} << 24) | ((units - 1) << 32), 8);
    ImageWriter::Put(result, 8, outerBlocks | ((recordCount - 2) << 24), 8);
    for (std::size_t i = 0; i < image.Files.size(); ++i) ImageWriter::Put(result, static_cast<std::size_t>(fileStart + i * 6), image.Files[i].Offset, 5);
    ImageWriter::Put(result, static_cast<std::size_t>(fileStart + image.Files.size() * 6), image.MetadataOffset, 5);
    ImageWriter::Put(result, static_cast<std::size_t>(fileStart + (fileCount - 1) * 6), image.Size | (std::uint64_t{0x40} << 40), 6);
    for (std::uint64_t group = 0; group < groups; ++group) {
        const auto base = 2 * std::min(group * 8, units) + 1;
        const auto offset = static_cast<std::size_t>(indexStart + group * 10);
        ImageWriter::Put(result, offset, base, 3);
        for (std::uint64_t i = 1; i < 8; ++i) ImageWriter::Put(result, offset + 2 + static_cast<std::size_t>(i), 2 * std::min(group * 8 + i, units) + 1 - base, 1);
    }
    for (std::uint64_t i = 0; i <= units; ++i) {
        const auto offset = std::min(i * unit, image.Size);
        const bool terminal = i == units;
        const auto runIndex = 2 * (offset / unit);
        const auto tweak = terminal ? 0 : offset >> 15;
        if (tweak > 0xfffffff || runIndex > 0x7fffff) throw std::length_error("NAPS run base exceeds packed range");
        const auto at = static_cast<std::size_t>(recordStart + i * 18);
        const auto low = (offset % unit) | (std::uint64_t{1} << 18) | (tweak << 19) | ((runIndex & 0x7fff) << 49);
        ImageWriter::Put(result, at, low, 8);
        ImageWriter::Put(result, at + 8, runIndex >> 15, 1);
        std::uint64_t standard = offset % unit;
        if (terminal) {
            standard |= (std::uint64_t{1} << 19) | ((offset % unit) << 20);
        } else {
            const auto size = std::min(unit, image.Size - offset);
            const auto half = std::min<std::uint64_t>(0x20000, size);
            standard |= ((half - 1) << 38) | (std::uint64_t{1} << 55);
            if (size > 0x20000) standard |= std::uint64_t{4} << 56;
        }
        ImageWriter::Put(result, at + 9, standard, 8);
    }
    return result;
}

}
