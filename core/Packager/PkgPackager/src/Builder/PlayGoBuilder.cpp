#include "Builder/PlayGoBuilder.hpp"
#include "Writer/FlatPathWriter.hpp"
#include "Writer/ImageWriter.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace AnyDevKit::Pkg {

PlayGoData PlayGoBuilder::Build(std::string_view contentId, const InnerImageInfo& inner, const OuterImageInfo& outer) {
    using Pfs::ImageWriter;
    if (contentId.size() != 36 || inner.Files.empty() || inner.Files.size() > 0x1fffffff || !inner.Size || inner.Size >= ImageWriter::Add(outer.Size, 0x10000)) throw std::invalid_argument("Invalid PlayGo inputs");
    PlayGoData result;
    result.Chunks.resize(0x1a0);
    auto& chunks = result.Chunks;
    const auto put = [&](std::size_t offset, std::uint64_t value, std::size_t size) { ImageWriter::Put(chunks, offset, value, size); };
    put(0, 0x78676c70, 4);
    put(4, 0x1000, 2);
    for (const auto offset : {8, 10, 14, 22}) put(offset, 1, 2);
    put(0x10, chunks.size(), 4);
    put(0x1e, 0x85, 1);
    put(0x20, 2, 4);
    put(0x24, 1, 4);
    put(0x30, 0x11, 4);
    put(0x38, 0xffffffffffffffff, 8);
    for (std::size_t i = 0; i < contentId.size(); ++i) chunks[0x40 + i] = static_cast<std::byte>(contentId[i]);
    constexpr std::array<std::pair<std::uint32_t, std::uint32_t>, 7> sections{{{0x100, 0x20}, {0x120, 8}, {0x130, 9}, {0x140, 0x20}, {0x160, 0x20}, {0x180, 2}, {0x190, 12}}};
    for (std::size_t i = 0; i < sections.size(); ++i) {
        put(0xc0 + i * 8, sections[i].first, 4);
        put(0xc4 + i * 8, sections[i].second, 4);
    }
    put(0x100, 0x80, 1);
    put(0x102, 3, 1);
    put(0x104, 2, 2);
    put(0x108, 0x11, 2);
    put(0x110, 0xffffffffffffffff, 8);
    put(0x124, 1, 4);
    put(0x148, inner.Size, 8);
    put(0x150, inner.Size, 8);
    put(0x158, ImageWriter::Add(outer.Size, 0x10000) - inner.Size, 8);
    put(0x160, 0x21, 8);
    put(0x174, 1, 2);
    put(0x176, 1, 2);
    const auto label = [&](std::size_t offset, std::string_view text) { for (std::size_t i = 0; i < text.size(); ++i) chunks[offset + i] = static_cast<std::byte>(text[i]); };
    label(0x130, "Chunk #0");
    label(0x190, "Scenario #0");
    std::vector<std::uint64_t> hashes;
    for (const auto& file : inner.Files) hashes.push_back(Pfs::FlatPathWriter::Hash(file.Path));
    std::sort(hashes.begin(), hashes.end());
    if (std::adjacent_find(hashes.begin(), hashes.end()) != hashes.end()) throw std::invalid_argument("PlayGo path hash collision");
    auto& table = result.Hashes;
    table.resize(0x38 + hashes.size() * 8);
    ImageWriter::Put(table, 0, 1, 4);
    ImageWriter::Put(table, 4, 0x08000000, 4);
    ImageWriter::Put(table, 8, 0x38, 4);
    ImageWriter::Put(table, 12, hashes.size() * 8, 4);
    ImageWriter::Put(table, 0x18, 0x544c467f, 4);
    ImageWriter::Put(table, 0x24, hashes.size(), 4);
    ImageWriter::Put(table, 0x28, 0x92ca8aab26a24f51, 8);
    ImageWriter::Put(table, 0x30, 0x09bbb761a41bc44d, 8);
    for (std::size_t i = 0; i < hashes.size(); ++i) ImageWriter::Put(table, 0x38 + i * 8, hashes[i], 8);
    result.FileChunks.resize(16 + hashes.size() * 2);
    ImageWriter::Put(result.FileChunks, 0, 1, 4);
    ImageWriter::Put(result.FileChunks, 8, 16, 4);
    ImageWriter::Put(result.FileChunks, 12, hashes.size() * 2, 4);
    return result;
}

}
