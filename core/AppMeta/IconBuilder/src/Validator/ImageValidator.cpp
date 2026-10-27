#include "Validator/ImageValidator.hpp"

#include <limits>
#include <stdexcept>

namespace AnyDevKit::Icon {

std::size_t ImageValidator::ComputePixelBytes(std::uint32_t width, std::uint32_t height) {
    if (!width || !height) throw std::invalid_argument("Image dimensions must be positive");
    if (width > 0x7fffffff || height > 0x7fffffff) throw std::invalid_argument("Image dimensions exceed format limits");
    if (width > std::numeric_limits<std::size_t>::max() / 4 / height) throw std::length_error("Image pixel buffer size overflow");
    return static_cast<std::size_t>(width) * height * 4;
}

void ImageValidator::Validate(const RgbaImage& image) {
    if (image.Pixels.size() != ComputePixelBytes(image.Width, image.Height)) throw std::invalid_argument("RGBA pixel buffer size does not match image dimensions");
}

}
