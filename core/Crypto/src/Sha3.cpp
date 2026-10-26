#include "Sha3.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>

namespace AnyDevKit::Crypto {

void Sha3::permute() {
    constexpr std::array<std::uint64_t, 24> constants{
        0x0000000000000001, 0x0000000000008082, 0x800000000000808a, 0x8000000080008000,
        0x000000000000808b, 0x0000000080000001, 0x8000000080008081, 0x8000000000008009,
        0x000000000000008a, 0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
        0x000000008000808b, 0x800000000000008b, 0x8000000000008089, 0x8000000000008003,
        0x8000000000008002, 0x8000000000000080, 0x000000000000800a, 0x800000008000000a,
        0x8000000080008081, 0x8000000000008080, 0x0000000080000001, 0x8000000080008008
    };
    constexpr std::array<int, 25> rotations{0, 1, 62, 28, 27, 36, 44, 6, 55, 20, 3, 10, 43, 25, 39, 41, 45, 15, 21, 8, 18, 2, 61, 56, 14};
    for (const auto constant : constants) {
        std::array<std::uint64_t, 5> columns{};
        for (std::size_t x = 0; x < 5; ++x) for (std::size_t y = 0; y < 5; ++y) columns[x] ^= state[x + 5 * y];
        for (std::size_t x = 0; x < 5; ++x) {
            const auto delta = columns[(x + 4) % 5] ^ std::rotl(columns[(x + 1) % 5], 1);
            for (std::size_t y = 0; y < 5; ++y) state[x + 5 * y] ^= delta;
        }
        std::array<std::uint64_t, 25> transformed{};
        for (std::size_t x = 0; x < 5; ++x) for (std::size_t y = 0; y < 5; ++y) transformed[y + 5 * ((2 * x + 3 * y) % 5)] = std::rotl(state[x + 5 * y], rotations[x + 5 * y]);
        for (std::size_t x = 0; x < 5; ++x) for (std::size_t y = 0; y < 5; ++y) state[x + 5 * y] = transformed[x + 5 * y] ^ (~transformed[(x + 1) % 5 + 5 * y] & transformed[(x + 2) % 5 + 5 * y]);
        state[0] ^= constant;
    }
}

void Sha3::Update(std::span<const std::byte> bytes) {
    if (finished) throw std::logic_error("SHA3 is finalized");
    for (const auto byte : bytes) {
        state[position / 8] ^= std::to_integer<std::uint64_t>(byte) << (8 * (position % 8));
        if (++position == 136) {
            permute();
            position = 0;
        }
    }
}

std::array<std::byte, 32> Sha3::Finish() {
    if (finished) throw std::logic_error("SHA3 is finalized");
    finished = true;
    state[position / 8] ^= std::uint64_t{6} << (8 * (position % 8));
    state[16] ^= 0x8000000000000000;
    permute();
    std::array<std::byte, 32> digest{};
    for (std::size_t i = 0; i < digest.size(); ++i) digest[i] = static_cast<std::byte>((state[i / 8] >> (8 * (i % 8))) & 0xff);
    return digest;
}

std::array<std::byte, 32> Sha3::Compute(std::span<const std::byte> bytes) {
    Sha3 hash;
    hash.Update(bytes);
    return hash.Finish();
}

void Sha3::Shake128(std::span<const std::byte> bytes, std::span<std::byte> output) {
    Sha3 sponge;
    for (const auto byte : bytes) {
        sponge.state[sponge.position / 8] ^= std::to_integer<std::uint64_t>(byte) << (8 * (sponge.position % 8));
        if (++sponge.position == 168) {
            sponge.permute();
            sponge.position = 0;
        }
    }
    sponge.state[sponge.position / 8] ^= std::uint64_t{0x1f} << (8 * (sponge.position % 8));
    sponge.state[20] ^= 0x8000000000000000;
    sponge.permute();
    sponge.position = 0;
    for (auto& byte : output) {
        if (sponge.position == 168) {
            sponge.permute();
            sponge.position = 0;
        }
        byte = static_cast<std::byte>((sponge.state[sponge.position / 8] >> (8 * (sponge.position % 8))) & 0xff);
        ++sponge.position;
    }
}

std::array<std::byte, 32> Sha3::Hmac(std::span<const std::byte> key, std::span<const std::byte> bytes) {
    std::array<std::byte, 136> block{};
    if (key.size() > block.size()) {
        const auto reduced = Compute(key);
        std::copy(reduced.begin(), reduced.end(), block.begin());
    } else std::copy(key.begin(), key.end(), block.begin());
    for (auto& byte : block) byte ^= std::byte{0x36};
    Sha3 inner;
    inner.Update(block);
    inner.Update(bytes);
    const auto digest = inner.Finish();
    for (auto& byte : block) byte ^= std::byte{0x36 ^ 0x5c};
    Sha3 outer;
    outer.Update(block);
    outer.Update(digest);
    return outer.Finish();
}

}
