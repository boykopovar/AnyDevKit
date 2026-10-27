#ifndef ANYDEVKIT_APPMETA_BC7ENCODER_HPP
#define ANYDEVKIT_APPMETA_BC7ENCODER_HPP

#include <array>
#include <cstddef>
#include <span>

namespace AnyDevKit::Icon {

class Bc7Encoder final {
public:
    static std::array<std::byte, 16> Encode(std::span<const std::byte, 64> pixels);
};

}

#endif
