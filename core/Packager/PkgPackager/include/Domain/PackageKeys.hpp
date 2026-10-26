#ifndef ANYDEVKIT_PKG_PACKAGEKEYS_HPP
#define ANYDEVKIT_PKG_PACKAGEKEYS_HPP

#include <array>
#include <cstddef>
#include <functional>
#include <span>

namespace AnyDevKit::Pkg {

struct PackageKeys {
    std::array<std::array<std::byte, 384>, 7> PasscodeModuli{};
    std::array<std::byte, 384> MountModulus{};
    std::array<std::byte, 384> MetadataModulus{};
    std::array<std::byte, 32> Passcode{};
    std::array<std::byte, 16> ImageSeed{};
    std::function<void(std::span<std::byte>)> FillRandom;
};

}

#endif
