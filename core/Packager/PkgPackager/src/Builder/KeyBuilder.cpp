#include "Builder/KeyBuilder.hpp"
#include "Rsa3072.hpp"
#include "Sha3.hpp"

#include <algorithm>
#include <stdexcept>

namespace AnyDevKit::Pkg {

std::array<std::byte, 384> KeyBuilder::Wrap(std::span<const std::byte> message, std::span<const std::byte, 384> modulus, const PackageKeys& keys) {
    if (!keys.FillRandom) throw std::invalid_argument("PKG requires a cryptographic random source");
    if (message.size() > 373) throw std::length_error("PKG RSA payload exceeds padding limit");
    std::vector<std::byte> padding(381 - message.size());
    keys.FillRandom(padding);
    for (auto& value : padding) {
        for (unsigned attempt = 0; value == std::byte{} && attempt < 256; ++attempt) keys.FillRandom(std::span(&value, 1));
        if (value == std::byte{}) throw std::runtime_error("PKG random source failed to produce nonzero RSA padding");
    }
    return Crypto::Rsa3072::Encrypt(message, modulus, 65537, padding);
}

PackageKeyData KeyBuilder::Build(std::string_view contentId, const PackageKeys& keys) {
    if (contentId.size() != 36) throw std::invalid_argument("PKG content ID must contain 36 characters");
    for (const auto c : contentId) if (c < 0x20 || c > 0x7e) throw std::invalid_argument("Invalid PKG content ID character");
    for (const auto c : keys.Passcode) if (c < std::byte{'0'} || (c > std::byte{'9'} && c < std::byte{'A'}) || (c > std::byte{'F'} && c < std::byte{'a'}) || c > std::byte{'f'}) throw std::invalid_argument("PKG passcode must contain 32 hexadecimal ASCII characters");
    std::array<std::byte, 48> id{};
    for (std::size_t i = 0; i < contentId.size(); ++i) id[i] = static_cast<std::byte>(contentId[i]);
    const auto idHash = Crypto::Sha3::Compute(id);
    PackageKeyData result;
    result.EntryKeys.resize(32 + 7 * (32 + 384));
    std::copy(idHash.begin(), idHash.end(), result.EntryKeys.begin());
    for (unsigned i = 0; i < 7; ++i) {
        const std::array index{std::byte{}, std::byte{}, std::byte{}, static_cast<std::byte>(i)};
        Crypto::Sha3 digest;
        digest.Update(Crypto::Sha3::Compute(index));
        digest.Update(idHash);
        digest.Update(keys.Passcode);
        const auto key = digest.Finish();
        if (i == 1) result.ImageSecret = key;
        const auto checksum = Crypto::Sha3::Compute(key);
        for (std::size_t j = 0; j < key.size(); ++j) result.EntryKeys[32 + i * 32 + j] = key[j] ^ checksum[j];
        const auto encrypted = Wrap(i == 0 ? std::span<const std::byte>(keys.Passcode) : std::span<const std::byte>(key), keys.PasscodeModuli[i], keys);
        std::copy(encrypted.begin(), encrypted.end(), result.EntryKeys.begin() + 256 + i * 384);
    }
    result.ImageKey.reserve(2048);
    while (result.ImageKey.size() < 2048) {
        const auto encrypted = Wrap(result.ImageSecret, keys.MountModulus, keys);
        const auto size = std::min(encrypted.size(), 2048 - result.ImageKey.size());
        result.ImageKey.insert(result.ImageKey.end(), encrypted.begin(), encrypted.begin() + static_cast<std::ptrdiff_t>(size));
    }
    return result;
}

}
