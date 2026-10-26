#include "Writer/FlatPathWriter.hpp"
#include "Writer/ImageWriter.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <stdexcept>

namespace AnyDevKit::Pfs {

std::uint64_t FlatPathWriter::Hash(std::string_view path) {
    if (path.starts_with('/')) path.remove_prefix(1);
    constexpr std::uint64_t seed = 0x92ca8aab26a24f51;
    constexpr std::uint64_t finalSeed = 0x09bbb761a41bc44d;
    constexpr std::uint64_t constant = 0x8000000080008081;
    std::array<std::uint64_t, 3> state{seed, std::rotl(seed, 11), std::rotl(seed, 23)};
    std::size_t offset = 0;
    const auto absorb = [&](std::size_t count) {
        std::uint64_t word = 0;
        for (std::size_t i = 0; i < count; ++i) {
            auto c = static_cast<unsigned char>(path[offset++]);
            if (c < 0x20 || c > 0x7e) throw std::invalid_argument("Unsupported flat path character");
            if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 'a' + 'A');
            word |= std::uint64_t{c} << (i * 8);
        }
        state[0] ^= word;
    };
    const auto round = [&] {
        const auto x = std::rotr(std::rotl(state[2] ^ state[1], 5) ^ state[0], 11);
        const auto y = std::rotl(std::rotl(state[2] ^ state[0], 17) ^ state[1], 11);
        const auto z = std::rotr(std::rotl(state[1] ^ state[0], 1) ^ state[2], 5);
        state = {x ^ (~y & z) ^ constant, y ^ (~z & x), z ^ (~x & y)};
    };
    while (path.size() - offset > 8) {
        absorb(8);
        round();
    }
    absorb(path.size() - offset);
    state[0] ^= finalSeed;
    round();
    return state[0];
}

std::vector<std::byte> FlatPathWriter::Build(std::span<const std::pair<std::uint64_t, std::uint64_t>> entries) {
    if (entries.size() > std::numeric_limits<std::uint32_t>::max() || entries.size() > (std::numeric_limits<std::size_t>::max() - 0x40) / 16) throw std::length_error("Flat path table is too large");
    std::vector<std::pair<std::uint64_t, std::uint64_t>> sorted(entries.begin(), entries.end());
    std::sort(sorted.begin(), sorted.end());
    for (std::size_t i = 1; i < sorted.size(); ++i) if (sorted[i].first == sorted[i - 1].first) throw std::invalid_argument("PS5 flat path hash collision");
    std::vector<std::byte> bytes(0x40 + sorted.size() * 16);
    ImageWriter::Put(bytes, 0, 1, 4);
    ImageWriter::Put(bytes, 4, 16, 4);
    ImageWriter::Put(bytes, 8, 0x40, 4);
    ImageWriter::Put(bytes, 0x20, 0x544c467f, 4);
    ImageWriter::Put(bytes, 0x2c, sorted.size(), 4);
    ImageWriter::Put(bytes, 0x30, 0x92ca8aab26a24f51, 8);
    ImageWriter::Put(bytes, 0x38, 0x09bbb761a41bc44d, 8);
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        ImageWriter::Put(bytes, 0x40 + i * 16, sorted[i].first, 8);
        ImageWriter::Put(bytes, 0x48 + i * 16, sorted[i].second, 8);
    }
    return bytes;
}

}
