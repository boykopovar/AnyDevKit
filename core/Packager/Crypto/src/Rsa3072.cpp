#include "Rsa3072.hpp"

#include <algorithm>
#include <stdexcept>

namespace AnyDevKit::Crypto {
namespace {

using Integer = std::array<std::uint32_t, 96>;

bool Less(const Integer& left, const Integer& right) {
    for (std::size_t i = left.size(); i-- > 0;) if (left[i] != right[i]) return left[i] < right[i];
    return false;
}

Integer Add(const Integer& left, const Integer& right, const Integer& modulus) {
    Integer result{};
    std::uint64_t carry = 0;
    for (std::size_t i = 0; i < result.size(); ++i) {
        const std::uint64_t value = std::uint64_t{left[i]} + right[i] + carry;
        result[i] = static_cast<std::uint32_t>(value);
        carry = value >> 32;
    }
    if (carry || !Less(result, modulus)) {
        std::uint64_t borrow = 0;
        for (std::size_t i = 0; i < result.size(); ++i) {
            const auto subtrahend = std::uint64_t{modulus[i]} + borrow;
            borrow = result[i] < subtrahend;
            result[i] = static_cast<std::uint32_t>(std::uint64_t{result[i]} - subtrahend);
        }
    }
    return result;
}

Integer Multiply(Integer left, const Integer& right, const Integer& modulus) {
    Integer result{};
    for (const auto word : right) for (unsigned bit = 0; bit < 32; ++bit) {
        if ((word >> bit) & 1) result = Add(result, left, modulus);
        left = Add(left, left, modulus);
    }
    return result;
}

Integer Decode(std::span<const std::byte, 384> bytes) {
    Integer result{};
    for (std::size_t i = 0; i < bytes.size(); ++i) result[i / 4] |= std::to_integer<std::uint32_t>(bytes[bytes.size() - i - 1]) << (8 * (i % 4));
    return result;
}

}

std::array<std::byte, 384> Rsa3072::ApplyPublic(std::span<const std::byte, 384> message, std::span<const std::byte, 384> modulus, std::uint32_t exponent) {
    if ((std::to_integer<unsigned>(modulus.front()) & 0x80) == 0 || (std::to_integer<unsigned>(modulus.back()) & 1) == 0 || exponent < 3 || !(exponent & 1)) throw std::invalid_argument("Invalid RSA-3072 public key");
    const auto divisor = Decode(modulus);
    auto base = Decode(message);
    if (!Less(base, divisor)) throw std::invalid_argument("RSA representative exceeds modulus");
    Integer result{};
    result[0] = 1;
    while (exponent) {
        if (exponent & 1) result = Multiply(result, base, divisor);
        exponent >>= 1;
        if (exponent) base = Multiply(base, base, divisor);
    }
    std::array<std::byte, 384> encoded{};
    for (std::size_t i = 0; i < encoded.size(); ++i) encoded[encoded.size() - i - 1] = static_cast<std::byte>((result[i / 4] >> (8 * (i % 4))) & 0xff);
    return encoded;
}

std::array<std::byte, 384> Rsa3072::Encrypt(std::span<const std::byte> message, std::span<const std::byte, 384> modulus, std::uint32_t exponent, std::span<const std::byte> padding) {
    if (message.size() > 373 || padding.size() != 381 - message.size()) throw std::invalid_argument("Invalid RSA PKCS1 message or padding length");
    if (std::find(padding.begin(), padding.end(), std::byte{0}) != padding.end()) throw std::invalid_argument("RSA PKCS1 padding contains zero");
    std::array<std::byte, 384> encoded{};
    encoded[1] = std::byte{2};
    std::copy(padding.begin(), padding.end(), encoded.begin() + 2);
    std::copy(message.begin(), message.end(), encoded.begin() + static_cast<std::ptrdiff_t>(3 + padding.size()));
    return ApplyPublic(encoded, modulus, exponent);
}

}
