#include "IconBuilder.hpp"

#include <lodepng.h>

#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

namespace AnyDevKit {
namespace {

std::size_t PixelBytes(std::uint32_t width, std::uint32_t height) {
    if (!width || !height) throw std::invalid_argument("PNG dimensions must be positive");
    if (width > 0x7fffffff || height > 0x7fffffff) throw std::invalid_argument("PNG dimensions exceed format limits");
    if (width > std::numeric_limits<std::size_t>::max() / 4 / height) throw std::length_error("PNG pixel buffer size overflow");
    return static_cast<std::size_t>(width) * height * 4;
}

void Check(unsigned error) {
    if (error) throw std::runtime_error("PNG error " + std::to_string(error) + ": " + lodepng_error_text(error));
}

}

RgbaImage IconBuilder::DecodePng(std::span<const std::byte> data) {
    if (data.empty()) throw std::invalid_argument("PNG input is empty");
    lodepng::State state;
    unsigned width = 0;
    unsigned height = 0;
    const auto* source = reinterpret_cast<const unsigned char*>(data.data());
    Check(lodepng_inspect(&width, &height, &state, source, data.size()));
    const auto expectedSize = PixelBytes(width, height);
    state.info_raw.colortype = LCT_RGBA;
    state.info_raw.bitdepth = 8;
    std::vector<unsigned char> pixels;
    Check(lodepng::decode(pixels, width, height, state, source, data.size()));
    if (pixels.size() != expectedSize) throw std::runtime_error("PNG decoded pixel size mismatch");
    RgbaImage result{width, height, std::vector<std::byte>(expectedSize)};
    std::memcpy(result.Pixels.data(), pixels.data(), pixels.size());
    return result;
}

RgbaImage IconBuilder::ReadPng(const std::filesystem::path& input) {
    const auto size = std::filesystem::file_size(input);
    if (size > std::numeric_limits<std::size_t>::max() || size > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) throw std::length_error("PNG file exceeds stream limits");
    std::vector<std::byte> data(static_cast<std::size_t>(size));
    std::ifstream stream;
    stream.exceptions(std::ios::badbit | std::ios::failbit);
    stream.open(input, std::ios::binary);
    if (!data.empty()) stream.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (stream.rdbuf()->sgetc() != std::char_traits<char>::eof()) throw std::runtime_error("PNG file grew while reading");
    stream.close();
    return DecodePng(data);
}

std::vector<std::byte> IconBuilder::EncodePng(const RgbaImage& image) {
    if (image.Pixels.size() != PixelBytes(image.Width, image.Height)) throw std::invalid_argument("RGBA pixel buffer size does not match image dimensions");
    std::vector<unsigned char> encoded;
    Check(lodepng::encode(encoded, reinterpret_cast<const unsigned char*>(image.Pixels.data()), image.Width, image.Height, LCT_RGBA, 8));
    std::vector<std::byte> result(encoded.size());
    std::memcpy(result.data(), encoded.data(), encoded.size());
    return result;
}

void IconBuilder::WritePng(const std::filesystem::path& output, const RgbaImage& image) {
    const auto encoded = EncodePng(image);
    if (encoded.size() > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) throw std::length_error("PNG output exceeds stream limits");
    std::ofstream stream;
    stream.exceptions(std::ios::badbit | std::ios::failbit);
    stream.open(output, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(encoded.data()), static_cast<std::streamsize>(encoded.size()));
    stream.close();
}

}
