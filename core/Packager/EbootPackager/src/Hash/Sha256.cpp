#include "Hash/Sha256.hpp"

#include <bit>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace AnyDevKit::Self {
namespace {

constexpr std::array<std::uint32_t, 64> Constants{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

void compress(std::array<std::uint32_t, 8>& state, std::span<const std::byte, 64> block) {
    std::array<std::uint32_t, 64> words{};
    for (std::size_t i = 0; i < 16; ++i) {
        for (std::size_t j = 0; j < 4; ++j) words[i] = (words[i] << 8) | std::to_integer<std::uint32_t>(block[i * 4 + j]);
    }
    for (std::size_t i = 16; i < words.size(); ++i) {
        const auto x = words[i - 15];
        const auto y = words[i - 2];
        words[i] = words[i - 16] + (std::rotr(x, 7) ^ std::rotr(x, 18) ^ (x >> 3)) + words[i - 7] + (std::rotr(y, 17) ^ std::rotr(y, 19) ^ (y >> 10));
    }
    auto [a, b, c, d, e, f, g, h] = state;
    for (std::size_t i = 0; i < words.size(); ++i) {
        const auto first = h + (std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25)) + ((e & f) ^ (~e & g)) + Constants[i] + words[i];
        const auto second = (std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        h = g;
        g = f;
        f = e;
        e = d + first;
        d = c;
        c = b;
        b = a;
        a = first + second;
    }
    const std::array updated{a, b, c, d, e, f, g, h};
    for (std::size_t i = 0; i < state.size(); ++i) state[i] += updated[i];
}

}

std::array<std::byte, 32> Sha256::Compute(std::span<const std::byte> bytes) {
    if (bytes.size() > std::numeric_limits<std::uint64_t>::max() / 8) throw std::length_error("SHA-256 input is too large");
    std::array<std::uint32_t, 8> state{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    const auto fullSize = bytes.size() - bytes.size() % 64;
    for (std::size_t offset = 0; offset < fullSize; offset += 64) compress(state, bytes.subspan(offset, 64).first<64>());
    std::array<std::byte, 128> tail{};
    const auto remaining = bytes.size() - fullSize;
    for (std::size_t i = 0; i < remaining; ++i) tail[i] = bytes[fullSize + i];
    tail[remaining] = std::byte{0x80};
    const std::size_t tailSize = remaining < 56 ? 64 : 128;
    const auto bitSize = static_cast<std::uint64_t>(bytes.size()) * 8;
    for (std::size_t i = 0; i < 8; ++i) tail[tailSize - 1 - i] = static_cast<std::byte>((bitSize >> (i * 8)) & 0xff);
    compress(state, std::span(tail).first<64>());
    if (tailSize == 128) compress(state, std::span(tail).last<64>());
    std::array<std::byte, 32> result{};
    for (std::size_t i = 0; i < result.size(); ++i) result[i] = static_cast<std::byte>((state[i / 4] >> ((3 - i % 4) * 8)) & 0xff);
    return result;
}

}
