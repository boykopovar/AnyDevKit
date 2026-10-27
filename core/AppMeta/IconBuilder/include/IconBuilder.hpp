#ifndef ANYDEVKIT_APPMETA_ICONBUILDER_HPP
#define ANYDEVKIT_APPMETA_ICONBUILDER_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace AnyDevKit {

struct RgbaImage {
    std::uint32_t Width = 0;
    std::uint32_t Height = 0;
    std::vector<std::byte> Pixels;
};

class IconBuilder final {
public:
    static RgbaImage DecodePng(std::span<const std::byte> data);
    static RgbaImage ReadPng(const std::filesystem::path& input);
    static std::vector<std::byte> EncodePng(const RgbaImage& image);
    static void WritePng(const std::filesystem::path& output, const RgbaImage& image);
    static std::vector<std::byte> EncodeDds(const RgbaImage& image);
    static void WriteDds(const std::filesystem::path& output, const RgbaImage& image);
private:
    static void writeFile(const std::filesystem::path& output, std::span<const std::byte> encoded);
};

}

#endif
