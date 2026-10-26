#include "Writer/FihWriter.hpp"
#include "Writer/ImageWriter.hpp"
#include "Sha3.hpp"

#include <algorithm>
#include <stdexcept>

namespace AnyDevKit::Pkg {

std::vector<std::byte> FihWriter::Build(const InnerImageInfo& inner, const OuterImageInfo& outer, std::span<const std::byte> naps, std::uint32_t contentVersion) {
    using Pfs::ImageWriter;
    if (!inner.Size || inner.Size % 0x10000 || inner.Size / 0x10000 > 0xffffffff || inner.InodeCount < 5 || !outer.Size || outer.Size % 0x10000 || std::uint64_t{outer.Superblock} * 0x10000 >= outer.Size || naps.empty()) throw std::invalid_argument("Invalid finalized image geometry");
    std::vector<std::byte> data(0x10000);
    ImageWriter::Put(data, 0, 0x4849467f, 4);
    ImageWriter::Put(data, 4, 1, 1);
    ImageWriter::Put(data, 6, 3, 1);
    ImageWriter::Put(data, 8, 1, 4);
    ImageWriter::Put(data, 0x10, 0x10000, 8);
    ImageWriter::Put(data, 0x18, outer.Size, 8);
    ImageWriter::Put(data, 0x20, (std::uint64_t{outer.Superblock} + 1) * 0x10000, 8);
    ImageWriter::Put(data, 0x28, 0x10000, 8);
    for (const auto offset : {0x30, 0x70, 0xd0}) std::copy(outer.SuperblockDigest.begin(), outer.SuperblockDigest.end(), data.begin() + offset);
    ImageWriter::Put(data, 0x50, inner.MetadataOffset / 0x10000, 8);
    ImageWriter::Put(data, 0x58, ImageWriter::Add(0x10000, outer.Size), 8);
    ImageWriter::Put(data, 0x60, 0x10000, 8);
    ImageWriter::Put(data, 0x68, 0x800000000000, 8);
    ImageWriter::Put(data, 0x90, inner.Size / 0x10000, 4);
    ImageWriter::Put(data, 0x94, inner.Files.size() + 1, 4);
    ImageWriter::Put(data, 0x98, inner.Files.size() + 1, 4);
    ImageWriter::Put(data, 0x9c, contentVersion, 4);
    ImageWriter::Put(data, 0xa0, inner.Size, 8);
    ImageWriter::Put(data, 0xa8, naps.size(), 8);
    const auto digest = Crypto::Sha3::Compute(naps);
    std::copy(digest.begin(), digest.end(), data.begin() + 0xb0);
    ImageWriter::Put(data, 0xf0, inner.ApplicationFiles, 4);
    ImageWriter::Put(data, 0xf8, 2, 4);
    return data;
}

}
