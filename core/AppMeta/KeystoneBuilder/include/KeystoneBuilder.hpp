#ifndef ANYDEVKIT_APPMETA_KEYSTONEBUILDER_HPP
#define ANYDEVKIT_APPMETA_KEYSTONEBUILDER_HPP

#include <array>
#include <cstddef>
#include <span>

namespace AnyDevKit {

class KeystoneBuilder final {
public:
    static std::array<std::byte, 96> Build(std::span<const std::byte, 32> passcode);
};

}

#endif
