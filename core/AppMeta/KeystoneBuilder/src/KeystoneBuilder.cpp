#include "KeystoneBuilder.hpp"
#include "Sha256.hpp"
#include <algorithm>
#include <stdexcept>

namespace AnyDevKit {

std::array<std::byte, 96> KeystoneBuilder::Build(std::span<const std::byte, 32> passcode) {
    for (const auto value : passcode) {
        const auto character = std::to_integer<unsigned char>(value);
        if (!((character >= '0' && character <= '9') || (character >= 'a' && character <= 'f') || (character >= 'A' && character <= 'F'))) throw std::invalid_argument("Keystone passcode must contain 32 hexadecimal characters");
    }
    constexpr std::array<std::byte, 32> fingerprintKey{std::byte{0xfe}, std::byte{0x2f}, std::byte{0x86}, std::byte{0xdc}, std::byte{0x4c}, std::byte{0x11}, std::byte{0x0c}, std::byte{0x93}, std::byte{0xb1}, std::byte{0xf6}, std::byte{0x87}, std::byte{0xd2}, std::byte{0xda}, std::byte{0x21}, std::byte{0xb8}, std::byte{0x78}, std::byte{0x62}, std::byte{0xfd}, std::byte{0x16}, std::byte{0xe3}, std::byte{0x1d}, std::byte{0xc0}, std::byte{0xf0}, std::byte{0xa8}, std::byte{0x32}, std::byte{0x10}, std::byte{0x35}, std::byte{0xfd}, std::byte{0xdf}, std::byte{0x6c}, std::byte{0x56}, std::byte{0xb7}};
    constexpr std::array<std::byte, 32> signatureKey{std::byte{0x4b}, std::byte{0xac}, std::byte{0x10}, std::byte{0x05}, std::byte{0x83}, std::byte{0xdc}, std::byte{0x7e}, std::byte{0xd2}, std::byte{0x17}, std::byte{0x8d}, std::byte{0x0c}, std::byte{0xe1}, std::byte{0x99}, std::byte{0xfe}, std::byte{0xe9}, std::byte{0xa2}, std::byte{0xd7}, std::byte{0x2c}, std::byte{0x7b}, std::byte{0x7b}, std::byte{0x0a}, std::byte{0x10}, std::byte{0xb7}, std::byte{0x2f}, std::byte{0xe4}, std::byte{0xbe}, std::byte{0x51}, std::byte{0xd8}, std::byte{0xbc}, std::byte{0xa1}, std::byte{0x88}, std::byte{0x28}};
    std::array<std::byte, 96> result{};
    constexpr char magic[] = "keystone";
    for (std::size_t i = 0; i < 8; ++i) result[i] = static_cast<std::byte>(magic[i]);
    result[8] = std::byte{3};
    result[10] = std::byte{1};
    const auto fingerprint = Crypto::Sha256::Hmac(fingerprintKey, passcode);
    std::copy(fingerprint.begin(), fingerprint.end(), result.begin() + 32);
    const auto signature = Crypto::Sha256::Hmac(signatureKey, std::span(result).first(64));
    std::copy(signature.begin(), signature.end(), result.begin() + 64);
    return result;
}

}
