#include "Builder/InstallMetadataBuilder.hpp"
#include "Writer/ImageWriter.hpp"
#include "Aes.hpp"
#include "Crc32.hpp"
#include "RollingChecksum.hpp"
#include "Sha256.hpp"
#include "Sha3.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace AnyDevKit::Pkg {

InstallMetadata InstallMetadataBuilder::Build(const std::filesystem::path& imagePath, const InnerImageInfo& inner, std::span<const std::byte, 32> imageKey, std::span<const std::byte, 16> seed) {
    using Pfs::ImageWriter;
    constexpr std::uint64_t blockSize = 0x10000;
    constexpr std::uint64_t unit = 0x40000;
    if (inner.Size < 0x20000 || inner.Size % blockSize || inner.MetadataOffset % unit || inner.MetadataOffset >= inner.Size || std::filesystem::file_size(imagePath) != inner.Size) throw std::invalid_argument("Invalid install metadata image geometry");
    const auto physicalCount = static_cast<std::size_t>(inner.Size / blockSize);
    const auto mappingCount = static_cast<std::size_t>((inner.Size + unit - 1) / unit);
    const auto fileCount = inner.Files.size() + 1;
    InstallMetadata result;
    result.Descriptor.resize(48);
    ImageWriter::Put(result.Descriptor, 16, inner.Size - 0x20000, 8);
    ImageWriter::Put(result.Descriptor, 24, 1001, 8);
    ImageWriter::Put(result.Descriptor, 32, inner.Size - 0x20000, 8);
    ImageWriter::Put(result.Descriptor, 40, 0x20000, 8);
    const auto append = [&](std::string_view tag, unsigned version, std::span<const std::byte> content) {
        if (tag.size() != 4 || version > 255) throw std::invalid_argument("Invalid install record tag");
        const auto start = result.Metrics.size();
        const auto end = ImageWriter::Add(ImageWriter::Add(start, 16), content.size());
        if (end > 0x1000000) throw std::length_error("Install metadata exceeds AES-XTS data-unit limit");
        result.Metrics.resize(static_cast<std::size_t>(end));
        for (std::size_t i = 0; i < tag.size(); ++i) result.Metrics[start + i] = static_cast<std::byte>(tag[tag.size() - 1 - i]);
        ImageWriter::Put(result.Metrics, start + 4, version, 1);
        ImageWriter::Put(result.Metrics, start + 8, content.size(), 8);
        std::copy(content.begin(), content.end(), result.Metrics.begin() + static_cast<std::ptrdiff_t>(start + 16));
    };
    std::array<std::byte, 24> header{};
    ImageWriter::Put(header, 0, 1, 4);
    ImageWriter::Put(header, 4, 0x50, 4);
    ImageWriter::Put(header, 8, physicalCount, 4);
    ImageWriter::Put(header, 12, static_cast<std::uint32_t>(inner.Size - blockSize), 4);
    ImageWriter::Put(header, 16, 1, 4);
    ImageWriter::Put(header, 20, blockSize, 4);
    append("phdr", 1, header);
    std::vector<std::byte> files(fileCount * 24), types(fileCount * 56), names;
    for (std::size_t i = 0; i < fileCount; ++i) {
        const bool metadata = i == inner.Files.size();
        const auto offset = metadata ? inner.MetadataOffset : inner.Files[i].Offset;
        const auto size = metadata ? inner.Size - inner.MetadataOffset : inner.Files[i].Size;
        const auto count = std::max<std::uint64_t>(1, (size + unit - 1) / unit);
        const auto name = metadata ? std::string("*PFSmetadata") : inner.Files[i].Path;
        if (offset % unit) throw std::invalid_argument("Unaligned raw install file");
        ImageWriter::Put(files, i * 24, size, 8);
        ImageWriter::Put(files, i * 24 + 8, offset / unit, 4);
        ImageWriter::Put(files, i * 24 + 12, count, 4);
        ImageWriter::Put(files, i * 24 + 16, metadata ? 1001 : 0, 4);
        ImageWriter::Put(files, i * 24 + 20, !metadata && inner.Files[i].Executable ? 1 : 0, 4);
        ImageWriter::Put(types, i * 56, 1, 8);
        ImageWriter::Put(types, i * 56 + 8, size, 8);
        ImageWriter::Put(types, i * 56 + 16, size, 8);
        ImageWriter::Put(types, i * 56 + 24, count, 8);
        for (const auto c : name) names.push_back(static_cast<std::byte>(c));
        names.push_back(std::byte{});
    }
    append("file", 2, files);
    append("ftyp", 1, types);
    std::vector<std::byte> classes(mappingCount, std::byte{15}), mapping(mappingCount * 40), positions(mappingCount * 16), hashes(mappingCount * 48), rolling(mappingCount * 8), physicalHashes(physicalCount * 32), checks(physicalCount * 4);
    std::array<std::byte, 20> keyInput{};
    keyInput[0] = std::byte{1};
    std::copy(seed.begin(), seed.end(), keyInput.begin() + 4);
    const auto derived = Crypto::Sha256::Hmac(imageKey, keyInput);
    std::array<std::byte, 32> xtsKey{};
    std::copy_n(derived.begin() + 16, 16, xtsKey.begin());
    std::copy_n(derived.begin(), 16, xtsKey.begin() + 16);
    std::ifstream source;
    source.exceptions(std::ios::badbit | std::ios::failbit);
    source.open(imagePath, std::ios::binary);
    std::vector<std::byte> buffer(unit);
    std::size_t fileIndex = 0;
    for (std::size_t i = 0; i < mappingCount; ++i) {
        const auto offset = i * unit;
        const auto size = static_cast<std::size_t>(std::min(unit, inner.Size - offset));
        source.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(size));
        auto bytes = std::span(buffer).first(size);
        while (fileIndex + 1 < inner.Files.size() && inner.Files[fileIndex + 1].Offset <= offset) ++fileIndex;
        const bool metadata = offset >= inner.MetadataOffset;
        const auto& file = inner.Files.at(fileIndex);
        const bool inFile = !metadata && offset >= file.Offset && offset < ImageWriter::Align(ImageWriter::Add(file.Offset, std::max<std::uint64_t>(1, file.Size)), unit);
        if (inFile && file.Executable) classes[i] = std::byte{1};
        ImageWriter::Put(mapping, i * 40, offset, 8);
        ImageWriter::Put(mapping, i * 40 + 8, size, 4);
        ImageWriter::Put(mapping, i * 40 + 12, size, 4);
        const auto first = std::min<std::size_t>(0x20000, size);
        ImageWriter::Put(mapping, i * 40 + 16, first, 4);
        ImageWriter::Put(mapping, i * 40 + 20, size - first, 4);
        ImageWriter::Put(mapping, i * 40 + 24, offset / blockSize, 4);
        ImageWriter::Put(mapping, i * 40 + 32, 1, 4);
        ImageWriter::Put(mapping, i * 40 + 36, 0x40090000, 4);
        ImageWriter::Put(positions, i * 16, offset, 8);
        ImageWriter::Put(positions, i * 16 + 8, offset / blockSize, 8);
        ImageWriter::Put(hashes, i * 48, Crypto::RollingChecksum::ByteSums(bytes), 8);
        const auto digest = Crypto::Sha3::Compute(bytes);
        std::copy(digest.begin(), digest.end(), hashes.begin() + static_cast<std::ptrdiff_t>(i * 48 + 8));
        ImageWriter::Put(hashes, i * 48 + 40, metadata ? 1001 : 0, 8);
        ImageWriter::Put(rolling, i * 8, Crypto::RollingChecksum::WindowHash(bytes), 8);
        for (std::size_t p = 0; p < size; p += blockSize) {
            const auto index = static_cast<std::size_t>((offset + p) / blockSize);
            auto block = bytes.subspan(p, blockSize);
            const auto blockHash = Crypto::Sha3::Compute(block);
            std::copy(blockHash.begin(), blockHash.end(), physicalHashes.begin() + static_cast<std::ptrdiff_t>(index * 32));
            std::array<std::byte, 16> tweak{};
            ImageWriter::Put(tweak, 0, index, 8);
            Crypto::Aes::EncryptXts(block, xtsKey, tweak);
            ImageWriter::Put(checks, index * 4, Crypto::Crc32::Castagnoli(block), 4);
        }
    }
    if (source.rdbuf()->sgetc() != std::char_traits<char>::eof()) throw std::runtime_error("Inner image changed while building install metadata");
    source.close();
    append("ibcl", 1, classes);
    append("ibcl", 1, std::vector<std::byte>(mappingCount));
    append("i2ob", 1, mapping);
    append("i2op", 1, positions);
    append("ihsh", 1, hashes);
    append("rhsh", 1, rolling);
    append("fstr", 1, names);
    std::array<std::byte, 20> tweakRecord{};
    ImageWriter::Put(tweakRecord, 4, physicalCount, 4);
    append("twek", 1, tweakRecord);
    append("obdg", 1, physicalHashes);
    append("obcc", 1, checks);
    for (const auto tag : {"pgpl", "pgil", "pgpi", "pgpu"}) append(tag, 1, result.Descriptor);
    append("zero", 1, std::vector<std::byte>((16 - result.Metrics.size() % 16) % 16));
    constexpr std::array<unsigned char, 32> keyBytes{0x02, 0x2d, 0xca, 0xf6, 0xd1, 0x11, 0xe5, 0x8f, 0x25, 0x93, 0x6e, 0xf5, 0x46, 0x93, 0x45, 0xab, 0xad, 0xac, 0x16, 0x37, 0x60, 0xda, 0x51, 0x46, 0x98, 0xc2, 0x45, 0xab, 0x4c, 0x9c, 0x42, 0x6c};
    std::array<std::byte, 32> metricKey{};
    for (std::size_t i = 0; i < keyBytes.size(); ++i) metricKey[i] = static_cast<std::byte>(keyBytes[i]);
    std::array<std::byte, 16> metricTweak{};
    ImageWriter::Put(metricTweak, 0, 0x7d10ba3c, 4);
    Crypto::Aes::EncryptXts(result.Metrics, metricKey, metricTweak);
    return result;
}

}
