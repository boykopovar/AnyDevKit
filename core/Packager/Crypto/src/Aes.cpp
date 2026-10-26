#include "Aes.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>

namespace AnyDevKit::Crypto {
namespace {

constexpr std::uint8_t Multiply(std::uint8_t left, std::uint8_t right) {
    std::uint8_t product = 0;
    for (int bit = 0; bit < 8; ++bit) {
        if (right & 1) product ^= left;
        left = static_cast<std::uint8_t>((left << 1) ^ ((left & 0x80) ? 0x11b : 0));
        right >>= 1;
    }
    return product;
}

constexpr auto Substitution() {
    std::array<std::uint8_t, 256> table{};
    for (std::size_t i = 0; i < table.size(); ++i) {
        std::uint8_t inverse = 1;
        auto base = static_cast<std::uint8_t>(i);
        for (unsigned exponent = 254; exponent; exponent >>= 1) {
            if (exponent & 1) inverse = Multiply(inverse, base);
            base = Multiply(base, base);
        }
        table[i] = inverse ^ std::rotl(inverse, 1) ^ std::rotl(inverse, 2) ^ std::rotl(inverse, 3) ^ std::rotl(inverse, 4) ^ 0x63;
    }
    return table;
}

constexpr auto substitution = Substitution();
constexpr auto inverseSubstitution = [] {
    std::array<std::uint8_t, 256> table{};
    for (std::size_t i = 0; i < table.size(); ++i) table[substitution[i]] = static_cast<std::uint8_t>(i);
    return table;
}();

}

Aes::Aes(std::span<const std::byte> key) {
    if (key.size() != 16 && key.size() != 24 && key.size() != 32) throw std::invalid_argument("AES key must contain 16, 24 or 32 bytes");
    rounds = key.size() / 4 + 6;
    for (std::size_t i = 0; i < key.size(); ++i) roundKeys[i] = std::to_integer<std::uint8_t>(key[i]);
    std::uint8_t constant = 1;
    for (std::size_t offset = key.size(); offset < (rounds + 1) * 16; offset += 4) {
        std::array<std::uint8_t, 4> word{};
        std::copy_n(roundKeys.begin() + static_cast<std::ptrdiff_t>(offset - 4), 4, word.begin());
        if (offset % key.size() == 0) {
            std::rotate(word.begin(), word.begin() + 1, word.end());
            for (auto& byte : word) byte = substitution[byte];
            word[0] ^= constant;
            constant = Multiply(constant, 2);
        } else if (key.size() == 32 && offset % key.size() == 16) {
            for (auto& byte : word) byte = substitution[byte];
        }
        for (std::size_t i = 0; i < 4; ++i) roundKeys[offset + i] = roundKeys[offset + i - key.size()] ^ word[i];
    }
}

std::array<std::byte, 16> Aes::transform(std::span<const std::byte, 16> block, bool decrypt) const {
    std::array<std::uint8_t, 16> state{};
    for (std::size_t i = 0; i < 16; ++i) state[i] = std::to_integer<std::uint8_t>(block[i]);
    const auto addKey = [&](std::size_t round) { for (std::size_t i = 0; i < 16; ++i) state[i] ^= roundKeys[round * 16 + i]; };
    const auto shift = [&] {
        const auto copy = state;
        for (std::size_t row = 0; row < 4; ++row) for (std::size_t column = 0; column < 4; ++column) state[row + column * 4] = copy[row + ((column + (decrypt ? 4 - row : row)) % 4) * 4];
    };
    const auto mix = [&] {
        const std::array<std::uint8_t, 4> coefficients = decrypt ? std::array<std::uint8_t, 4>{14, 11, 13, 9} : std::array<std::uint8_t, 4>{2, 3, 1, 1};
        const auto copy = state;
        for (std::size_t column = 0; column < 4; ++column) for (std::size_t row = 0; row < 4; ++row) {
            std::uint8_t value = 0;
            for (std::size_t i = 0; i < 4; ++i) value ^= Multiply(copy[column * 4 + i], coefficients[(i + 4 - row) % 4]);
            state[column * 4 + row] = value;
        }
    };
    addKey(decrypt ? rounds : 0);
    for (std::size_t step = 1; step <= rounds; ++step) {
        if (decrypt) shift();
        for (auto& byte : state) byte = decrypt ? inverseSubstitution[byte] : substitution[byte];
        if (!decrypt) shift();
        if (!decrypt && step != rounds) mix();
        addKey(decrypt ? rounds - step : step);
        if (decrypt && step != rounds) mix();
    }
    std::array<std::byte, 16> result{};
    for (std::size_t i = 0; i < 16; ++i) result[i] = static_cast<std::byte>(state[i]);
    return result;
}

std::array<std::byte, 16> Aes::EncryptBlock(std::span<const std::byte, 16> block) const { return transform(block, false); }
std::array<std::byte, 16> Aes::DecryptBlock(std::span<const std::byte, 16> block) const { return transform(block, true); }

void Aes::EncryptCbc(std::span<std::byte> bytes, std::span<const std::byte, 16> iv) const {
    if (bytes.size() % 16) throw std::invalid_argument("CBC input must contain complete blocks");
    std::array<std::byte, 16> previous{};
    std::copy(iv.begin(), iv.end(), previous.begin());
    for (std::size_t offset = 0; offset < bytes.size(); offset += 16) {
        for (std::size_t i = 0; i < 16; ++i) previous[i] ^= bytes[offset + i];
        previous = EncryptBlock(previous);
        std::copy(previous.begin(), previous.end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset));
    }
}

void Aes::DecryptCbc(std::span<std::byte> bytes, std::span<const std::byte, 16> iv) const {
    if (bytes.size() % 16) throw std::invalid_argument("CBC input must contain complete blocks");
    std::array<std::byte, 16> previous{};
    std::copy(iv.begin(), iv.end(), previous.begin());
    for (std::size_t offset = 0; offset < bytes.size(); offset += 16) {
        std::array<std::byte, 16> encrypted{};
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset), 16, encrypted.begin());
        const auto plain = DecryptBlock(encrypted);
        for (std::size_t i = 0; i < 16; ++i) bytes[offset + i] = plain[i] ^ previous[i];
        previous = encrypted;
    }
}

void Aes::transformXts(std::span<std::byte> bytes, std::span<const std::byte> key, std::span<const std::byte, 16> tweak, bool decrypt) {
    if (key.size() != 32 && key.size() != 64) throw std::invalid_argument("XTS key must contain 32 or 64 bytes");
    if (bytes.size() < 16 || bytes.size() > 0x1000000) throw std::invalid_argument("Invalid XTS data unit length");
    const auto half = key.size() / 2;
    if (std::equal(key.begin(), key.begin() + static_cast<std::ptrdiff_t>(half), key.begin() + static_cast<std::ptrdiff_t>(half))) throw std::invalid_argument("XTS keys must differ");
    const Aes cipher(key.first(half));
    const Aes tweakCipher(key.subspan(half));
    auto current = tweakCipher.EncryptBlock(tweak);
    const auto advance = [](auto& value) {
        unsigned carry = 0;
        for (auto& byte : value) {
            const auto next = std::to_integer<unsigned>(byte);
            byte = static_cast<std::byte>(((next << 1) | carry) & 0xff);
            carry = next >> 7;
        }
        if (carry) value[0] ^= std::byte{0x87};
    };
    const auto process = [&](std::size_t offset, const auto& value) {
        std::array<std::byte, 16> block{};
        for (std::size_t i = 0; i < 16; ++i) block[i] = bytes[offset + i] ^ value[i];
        block = decrypt ? cipher.DecryptBlock(block) : cipher.EncryptBlock(block);
        for (std::size_t i = 0; i < 16; ++i) bytes[offset + i] = block[i] ^ value[i];
    };
    const auto tail = bytes.size() % 16;
    const auto complete = bytes.size() / 16 - (tail ? 1 : 0);
    for (std::size_t i = 0; i < complete; ++i) {
        process(i * 16, current);
        advance(current);
    }
    if (tail) {
        auto next = current;
        advance(next);
        const auto offset = complete * 16;
        process(offset, decrypt ? next : current);
        for (std::size_t i = 0; i < tail; ++i) std::swap(bytes[offset + i], bytes[offset + 16 + i]);
        process(offset, decrypt ? current : next);
    }
}

void Aes::EncryptXts(std::span<std::byte> bytes, std::span<const std::byte> key, std::span<const std::byte, 16> tweak) { transformXts(bytes, key, tweak, false); }
void Aes::DecryptXts(std::span<std::byte> bytes, std::span<const std::byte> key, std::span<const std::byte, 16> tweak) { transformXts(bytes, key, tweak, true); }

}
