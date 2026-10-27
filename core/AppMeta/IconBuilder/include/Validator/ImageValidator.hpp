#ifndef ANYDEVKIT_APPMETA_IMAGEVALIDATOR_HPP
#define ANYDEVKIT_APPMETA_IMAGEVALIDATOR_HPP

#include "IconBuilder.hpp"

namespace AnyDevKit::Icon {

class ImageValidator final {
public:
    static std::size_t ComputePixelBytes(std::uint32_t width, std::uint32_t height);
    static void Validate(const RgbaImage& image);
};

}

#endif
