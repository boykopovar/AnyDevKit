#include "DefaultIconGenerator.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace AnyDevKit {
namespace {

using Color = std::array<double, 3>;
constexpr std::uint32_t Extent = 512;
constexpr unsigned Samples = 4;

double segmentDistance(double x, double y, double startX, double startY, double endX, double endY) {
    const double dx = endX - startX;
    const double dy = endY - startY;
    const double t = std::clamp(((x - startX) * dx + (y - startY) * dy) / (dx * dx + dy * dy), 0.0, 1.0);
    return std::hypot(x - startX - t * dx, y - startY - t * dy);
}

Color sample(double x, double y) {
    const double horizontal = std::abs(x - 256.0) - 174.5;
    const double vertical = std::abs(y - 256.0) - 174.5;
    const double frameDistance = std::hypot(std::max(horizontal, 0.0), std::max(vertical, 0.0)) + std::min(std::max(horizontal, vertical), 0.0) - 48.0;
    if (std::abs(frameDistance) <= 1.5) return {61, 195, 179};
    if (segmentDistance(x, y, 188, 193, 125, 256) <= 11.5 || segmentDistance(x, y, 125, 256, 188, 319) <= 11.5 || segmentDistance(x, y, 324, 193, 387, 256) <= 11.5 || segmentDistance(x, y, 387, 256, 324, 319) <= 11.5) return {230, 243, 247};
    if (segmentDistance(x, y, 286, 170, 226, 342) <= 12.0) return {68, 218, 190};
    const double gradient = 1.0 - (x + y) / (2.0 * Extent);
    const double glow = std::max(0.0, 1.0 - std::hypot(x - 110.0, y - 90.0) / 470.0);
    return {12.0 + 10.0 * gradient + 2.0 * glow, 21.0 + 15.0 * gradient + 7.0 * glow, 34.0 + 18.0 * gradient + 9.0 * glow};
}

}

RgbaImage DefaultIconGenerator::Generate() {
    RgbaImage image{Extent, Extent, std::vector<std::byte>(static_cast<std::size_t>(Extent) * Extent * 4)};
    for (std::uint32_t y = 0; y < Extent; ++y) {
        for (std::uint32_t x = 0; x < Extent; ++x) {
            Color sum{};
            for (unsigned sampleY = 0; sampleY < Samples; ++sampleY) {
                for (unsigned sampleX = 0; sampleX < Samples; ++sampleX) {
                    const auto color = sample(x + (sampleX + 0.5) / Samples, y + (sampleY + 0.5) / Samples);
                    for (std::size_t channel = 0; channel < sum.size(); ++channel) sum[channel] += color[channel];
                }
            }
            const auto offset = (static_cast<std::size_t>(y) * Extent + x) * 4;
            for (std::size_t channel = 0; channel < sum.size(); ++channel) image.Pixels[offset + channel] = static_cast<std::byte>(std::lround(sum[channel] / (Samples * Samples)));
            image.Pixels[offset + 3] = std::byte{255};
        }
    }
    return image;
}

}
