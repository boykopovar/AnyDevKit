#include "Builder/OuterImageBuilder.hpp"
#include "Aes.hpp"
#include "Sha256.hpp"
#include "Sha3.hpp"
#include "Writer/DirectoryWriter.hpp"
#include "Writer/FlatPathWriter.hpp"
#include "Writer/ImageWriter.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <stdexcept>

namespace AnyDevKit::Pkg {

OuterImageInfo OuterImageBuilder::Build(const std::filesystem::path& inner, const std::filesystem::path& naps, std::uint64_t innerLogicalSize, std::span<const std::byte, 32> key, std::span<const std::byte, 16> seed, std::int64_t timestamp, const std::filesystem::path& output) {
    using Pfs::ImageWriter;
    constexpr std::uint64_t blockSize = 0x10000;
    if (timestamp < 0 || innerLogicalSize == 0 || innerLogicalSize % blockSize) throw std::invalid_argument("Invalid outer PFS geometry or timestamp");
    if (std::filesystem::exists(std::filesystem::symlink_status(output))) throw std::invalid_argument("Outer PFS output already exists");
    const std::array paths{inner, naps};
    std::array<std::uint64_t, 2> sizes{}, counts{}, starts{};
    for (std::size_t i = 0; i < paths.size(); ++i) {
        if (!std::filesystem::is_regular_file(std::filesystem::symlink_status(paths[i]))) throw std::invalid_argument("Outer PFS source is not a regular file");
        sizes[i] = std::filesystem::file_size(paths[i]);
        if (!sizes[i]) throw std::invalid_argument("Outer PFS source is empty");
        counts[i] = ImageWriter::Align(sizes[i], blockSize) / blockSize;
    }
    starts[1] = counts[0];
    const auto dataBlocks = ImageWriter::Add(counts[0], counts[1]);
    if (dataBlocks > 0x7fffff00) throw std::length_error("Outer PFS exceeds D32 block range");
    const auto superblock = static_cast<std::uint32_t>(dataBlocks);
    std::uint32_t next = superblock + 4;
    const auto derived = Crypto::Sha256::Hmac(key, seed);
    std::array<std::byte, 20> keyInput{};
    keyInput[0] = std::byte{1};
    std::copy(seed.begin(), seed.end(), keyInput.begin() + 4);
    const auto combined = Crypto::Sha256::Hmac(derived, keyInput);
    std::array<std::byte, 32> xtsKey{};
    std::copy_n(combined.begin() + 16, 16, xtsKey.begin());
    std::copy_n(combined.begin(), 16, xtsKey.begin() + 16);
    std::vector<std::array<std::byte, 32>> digests(static_cast<std::size_t>(next));
    try {
        ImageWriter writer(output);
        const auto writeBlock = [&](std::uint32_t index, std::span<const std::byte> data, bool signedBlock, bool plaintext) {
            if (data.size() > blockSize || index >= 0x7fffffff) throw std::length_error("Invalid outer PFS block");
            std::vector<std::byte> block(blockSize);
            std::copy(data.begin(), data.end(), block.begin());
            if (index >= digests.size()) digests.resize(static_cast<std::size_t>(index) + 1);
            digests[index] = Crypto::Sha3::Compute(block);
            if (!plaintext) {
                std::array<std::byte, 16> tweak{};
                ImageWriter::Put(tweak, 0, index | (signedBlock ? 0x800000000000ull : 0), 8);
                Crypto::Aes::EncryptXts(block, xtsKey, tweak);
            }
            writer.Write(std::uint64_t{index} * blockSize, block);
        };
        for (std::size_t i = 0; i < paths.size(); ++i) {
            std::ifstream stream;
            stream.exceptions(std::ios::badbit | std::ios::failbit);
            stream.open(paths[i], std::ios::binary);
            for (std::uint64_t block = 0; block < counts[i]; ++block) {
                std::array<std::byte, blockSize> bytes{};
                const auto amount = std::min(blockSize, sizes[i] - block * blockSize);
                stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(amount));
                writeBlock(static_cast<std::uint32_t>(starts[i] + block), bytes, i != 0, false);
            }
            if (stream.rdbuf()->sgetc() != std::char_traits<char>::eof()) throw std::runtime_error("Outer PFS source grew while reading");
            stream.close();
        }
        std::vector<std::byte> directory;
        Pfs::DirectoryWriter::Append(directory, 1, 2, "inode_flat_path_table");
        Pfs::DirectoryWriter::Append(directory, 2, 3, "uroot");
        writeBlock(superblock + 2, directory, true, false);
        const std::array<std::pair<std::uint64_t, std::uint64_t>, 2> flatEntries{{{Pfs::FlatPathWriter::Hash("pfs_image.dat"), 3}, {Pfs::FlatPathWriter::Hash("naps_pkg_layout.dat"), 4 | (std::uint64_t{1} << 40)}}};
        const auto flat = Pfs::FlatPathWriter::Build(flatEntries);
        writeBlock(superblock + 3, flat, true, false);
        const auto putPointer = [&](std::span<std::byte> bytes, std::size_t offset, std::uint32_t index) {
            if (index >= digests.size()) throw std::logic_error("Outer PFS pointer has no digest");
            std::copy(digests[index].begin(), digests[index].end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset));
            ImageWriter::Put(bytes, offset + 32, index, 4);
        };
        std::function<std::uint32_t(std::uint64_t&, std::uint64_t&, unsigned)> indirect;
        indirect = [&](std::uint64_t& first, std::uint64_t& remaining, unsigned depth) {
            if (!depth || !remaining) throw std::logic_error("Invalid outer PFS indirect tree");
            std::vector<std::byte> table(blockSize);
            for (std::size_t slot = 0; slot < blockSize / 36 && remaining; ++slot) {
                std::uint32_t child = 0;
                if (depth == 1) {
                    child = static_cast<std::uint32_t>(first++);
                    --remaining;
                } else child = indirect(first, remaining, depth - 1);
                putPointer(table, slot * 36, child);
            }
            if (next >= 0x7fffffff) throw std::length_error("Outer PFS indirect tree exceeds D32 range");
            const auto index = next++;
            writeBlock(index, table, true, false);
            return index;
        };
        std::vector<std::byte> inodeTable(blockSize);
        const auto inode = [&](std::size_t number, std::uint16_t mode, std::uint16_t links, std::uint32_t flags, std::uint64_t size, std::uint64_t logicalSize, std::uint64_t first, std::uint64_t count) {
            auto bytes = std::span(inodeTable).subspan(number * 0x2c8, 0x2c8);
            ImageWriter::Put(bytes, 0, mode, 2);
            ImageWriter::Put(bytes, 2, links, 2);
            ImageWriter::Put(bytes, 4, flags, 4);
            ImageWriter::Put(bytes, 8, size, 8);
            ImageWriter::Put(bytes, 16, logicalSize, 8);
            for (std::size_t i = 0; i < 4; ++i) ImageWriter::Put(bytes, 0x18 + i * 8, static_cast<std::uint64_t>(timestamp), 8);
            ImageWriter::Put(bytes, 0x60, count, 4);
            for (std::size_t slot = 0; slot < 12 && count; ++slot) {
                putPointer(bytes, 0x64 + slot * 36, static_cast<std::uint32_t>(first++));
                --count;
            }
            for (unsigned depth = 1; depth <= 5 && count; ++depth) putPointer(bytes, 0x64 + (11 + depth) * 36, indirect(first, count, depth));
            if (count) throw std::length_error("Outer PFS file exceeds inode capacity");
        };
        inode(0, 0x416d, 1, 0x2000c, blockSize, blockSize, superblock + 2, 1);
        inode(1, 0x816d, 1, 0x2000c, flat.size(), flat.size(), superblock + 3, 1);
        inode(3, 0x816d, 1, 0xd, sizes[0], innerLogicalSize, starts[0], counts[0]);
        inode(4, 0x816d, 1, 0xd, sizes[1], sizes[1], starts[1], counts[1]);
        directory.clear();
        Pfs::DirectoryWriter::Append(directory, 2, 4, ".");
        Pfs::DirectoryWriter::Append(directory, 2, 5, "..");
        Pfs::DirectoryWriter::Append(directory, 3, 2, "pfs_image.dat");
        Pfs::DirectoryWriter::Append(directory, 4, 2, "naps_pkg_layout.dat");
        const auto rootBlock = next++;
        writeBlock(rootBlock, directory, true, false);
        inode(2, 0x416d, 3, 0xc, blockSize, blockSize, rootBlock, 1);
        writeBlock(superblock + 1, inodeTable, true, false);
        std::vector<std::byte> header(blockSize);
        ImageWriter::Put(header, 0, 2, 8);
        ImageWriter::Put(header, 8, 20130315, 8);
        ImageWriter::Put(header, 0x1a, 1, 1);
        ImageWriter::Put(header, 0x1c, 0xd, 2);
        ImageWriter::Put(header, 0x20, blockSize, 4);
        ImageWriter::Put(header, 0x28, 1, 8);
        ImageWriter::Put(header, 0x30, 5, 8);
        ImageWriter::Put(header, 0x38, next, 8);
        ImageWriter::Put(header, 0x40, 1, 8);
        ImageWriter::Put(header, 0x52, 1, 2);
        ImageWriter::Put(header, 0x58, blockSize, 8);
        ImageWriter::Put(header, 0x60, blockSize, 8);
        for (std::size_t i = 0; i < 4; ++i) ImageWriter::Put(header, 0x68 + i * 8, static_cast<std::uint64_t>(timestamp), 8);
        ImageWriter::Put(header, 0xb0, 1, 4);
        putPointer(header, 0xb8, superblock + 1);
        ImageWriter::Put(header, 0x36c, 1, 4);
        std::copy(seed.begin(), seed.end(), header.begin() + 0x370);
        const auto integrity = Crypto::Sha3::Compute(std::span(header).first(0x5a0));
        std::copy(integrity.begin(), integrity.end(), header.begin() + 0x380);
        writeBlock(superblock, header, false, true);
        writer.Close();
        OuterImageInfo result;
        result.Size = std::uint64_t{next} * blockSize;
        result.Superblock = superblock;
        result.SuperblockDigest = digests[superblock];
        result.BlockDigests.reserve(digests.size() * 32);
        for (const auto& digest : digests) result.BlockDigests.insert(result.BlockDigests.end(), digest.rbegin(), digest.rend());
        return result;
    } catch (...) {
        std::error_code error;
        std::filesystem::remove(output, error);
        throw;
    }
}

}
