#include "Writer/DdsWriter.hpp"
#include "Encoder/Bc7Encoder.hpp"
#include "Validator/ImageValidator.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace AnyDevKit::Icon {

std::vector<std::byte> DdsWriter::Build(const RgbaImage& image) {
    ImageValidator::Validate(image);
    const auto columns = (std::uint64_t{image.Width} + 3) / 4;
    const auto rows = (std::uint64_t{image.Height} + 3) / 4;
    const auto payloadSize = columns * rows * 16;
    if (payloadSize > 0xffffffff || payloadSize > std::numeric_limits<std::size_t>::max() - 148) throw std::length_error("DDS payload exceeds format limits");
    std::vector<std::byte> result(static_cast<std::size_t>(148 + payloadSize));
    const auto put = [&](std::size_t offset, std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) result[offset + i] = static_cast<std::byte>((value >> (i * 8)) & 0xff);
    };
    put(0, 0x20534444);
    put(4, 124);
    put(8, 0xa1007);
    put(12, image.Height);
    put(16, image.Width);
    put(20, static_cast<std::uint32_t>(payloadSize));
    put(28, 1);
    put(76, 32);
    put(80, 4);
    put(84, 0x30315844);
    put(108, 0x1000);
    put(128, 98);
    put(132, 3);
    put(140, 1);
    std::size_t cursor = 148;
    for (std::uint64_t y = 0; y < rows; ++y) for (std::uint64_t x = 0; x < columns; ++x) {
        std::array<std::byte, 64> pixels{};
        for (std::uint32_t dy = 0; dy < 4; ++dy) for (std::uint32_t dx = 0; dx < 4; ++dx) {
            const auto sourceX = std::min<std::uint64_t>(x * 4 + dx, image.Width - 1);
            const auto sourceY = std::min<std::uint64_t>(y * 4 + dy, image.Height - 1);
            const auto offset = static_cast<std::size_t>((sourceY * image.Width + sourceX) * 4);
            std::copy_n(image.Pixels.begin() + static_cast<std::ptrdiff_t>(offset), 4, pixels.begin() + (dy * 4 + dx) * 4);
        }
        const auto encoded = Bc7Encoder::Encode(pixels);
        std::copy(encoded.begin(), encoded.end(), result.begin() + static_cast<std::ptrdiff_t>(cursor));
        cursor += encoded.size();
    }
    return result;
}

}
