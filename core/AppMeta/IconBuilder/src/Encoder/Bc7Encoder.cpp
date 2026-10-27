#include "Encoder/Bc7Encoder.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace AnyDevKit::Icon {
namespace {

using Color = std::array<int, 4>;
using Endpoint = std::array<double, 4>;
constexpr std::array<int, 16> weights{0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};

struct Encoding {
    std::array<Color, 2> Endpoints{};
    std::array<unsigned, 16> Indices{};
    unsigned Error = std::numeric_limits<unsigned>::max();
};

unsigned Distance(const Color& left, const Color& right) {
    unsigned result = 0;
    for (std::size_t c = 0; c < 4; ++c) {
        const auto delta = left[c] - right[c];
        result += static_cast<unsigned>(delta * delta);
    }
    return result;
}

Encoding Quantize(const std::array<Color, 16>& pixels, const std::array<Endpoint, 2>& endpoints) {
    Encoding best;
    const bool opaque = std::all_of(pixels.begin(), pixels.end(), [](const auto& pixel) { return pixel[3] == 255; });
    const bool transparent = std::all_of(pixels.begin(), pixels.end(), [](const auto& pixel) { return pixel[3] == 0; });
    for (unsigned parity = 0; parity < 4; ++parity) {
        if ((opaque && parity != 3) || (transparent && parity != 0)) continue;
        Encoding candidate;
        candidate.Error = 0;
        for (unsigned e = 0; e < 2; ++e) {
            const auto p = static_cast<int>((parity >> e) & 1);
            for (std::size_t c = 0; c < 4; ++c) candidate.Endpoints[e][c] = 2 * static_cast<int>(std::clamp(std::round((endpoints[e][c] - p) / 2), 0.0, 127.0)) + p;
        }
        std::array<Color, 16> palette{};
        for (std::size_t i = 0; i < palette.size(); ++i) for (std::size_t c = 0; c < 4; ++c) palette[i][c] = ((64 - weights[i]) * candidate.Endpoints[0][c] + weights[i] * candidate.Endpoints[1][c] + 32) >> 6;
        for (std::size_t i = 0; i < pixels.size(); ++i) {
            unsigned error = std::numeric_limits<unsigned>::max();
            for (unsigned index = 0; index < palette.size(); ++index) {
                const auto distance = Distance(pixels[i], palette[index]);
                if (distance < error) {
                    error = distance;
                    candidate.Indices[i] = index;
                }
            }
            candidate.Error += error;
        }
        if (candidate.Error < best.Error) best = candidate;
    }
    return best;
}

Encoding Optimize(const std::array<Color, 16>& pixels, std::array<Endpoint, 2> endpoints) {
    Encoding best;
    for (unsigned iteration = 0; iteration < 12; ++iteration) {
        const auto current = Quantize(pixels, endpoints);
        if (current.Error >= best.Error) break;
        best = current;
        if (!best.Error) break;
        double aa = 0, ab = 0, bb = 0;
        Endpoint left{}, right{};
        for (std::size_t i = 0; i < pixels.size(); ++i) {
            const double b = weights[current.Indices[i]] / 64.0;
            const double a = 1 - b;
            aa += a * a;
            ab += a * b;
            bb += b * b;
            for (std::size_t c = 0; c < 4; ++c) {
                left[c] += a * pixels[i][c];
                right[c] += b * pixels[i][c];
            }
        }
        const double determinant = aa * bb - ab * ab;
        if (determinant <= 1e-12) break;
        for (std::size_t c = 0; c < 4; ++c) {
            endpoints[0][c] = std::clamp((left[c] * bb - right[c] * ab) / determinant, 0.0, 255.0);
            endpoints[1][c] = std::clamp((right[c] * aa - left[c] * ab) / determinant, 0.0, 255.0);
        }
    }
    return best;
}

}

std::array<std::byte, 16> Bc7Encoder::Encode(std::span<const std::byte, 64> pixels) {
    std::array<Color, 16> colors{};
    for (std::size_t i = 0; i < colors.size(); ++i) for (std::size_t c = 0; c < 4; ++c) colors[i][c] = std::to_integer<int>(pixels[i * 4 + c]);
    std::array<Endpoint, 2> extremes{};
    for (std::size_t c = 0; c < 4; ++c) {
        extremes[0][c] = 255;
        for (const auto& color : colors) {
            extremes[0][c] = std::min(extremes[0][c], static_cast<double>(color[c]));
            extremes[1][c] = std::max(extremes[1][c], static_cast<double>(color[c]));
        }
    }
    auto best = Optimize(colors, extremes);
    unsigned largest = 0;
    std::size_t first = 0, second = 0;
    for (std::size_t i = 0; i < colors.size(); ++i) for (std::size_t j = i + 1; j < colors.size(); ++j) {
        const auto distance = Distance(colors[i], colors[j]);
        if (distance > largest) {
            largest = distance;
            first = i;
            second = j;
        }
    }
    for (std::size_t c = 0; c < 4; ++c) {
        extremes[0][c] = colors[first][c];
        extremes[1][c] = colors[second][c];
    }
    const auto alternate = Optimize(colors, extremes);
    if (alternate.Error < best.Error) best = alternate;
    if (best.Indices[0] >= 8) {
        std::swap(best.Endpoints[0], best.Endpoints[1]);
        for (auto& index : best.Indices) index = 15 - index;
    }
    std::array<std::byte, 16> result{};
    unsigned cursor = 0;
    const auto put = [&](unsigned value, unsigned bits) {
        if (bits > 7 || cursor + bits > 128 || value >= (1u << bits)) throw std::logic_error("Invalid BC7 packed field");
        for (unsigned i = 0; i < bits; ++i, ++cursor) if ((value >> i) & 1) result[cursor / 8] |= static_cast<std::byte>(1u << (cursor % 8));
    };
    put(0x40, 7);
    for (std::size_t c = 0; c < 4; ++c) for (unsigned e = 0; e < 2; ++e) put(static_cast<unsigned>(best.Endpoints[e][c] >> 1), 7);
    for (unsigned e = 0; e < 2; ++e) put(static_cast<unsigned>(best.Endpoints[e][0] & 1), 1);
    for (std::size_t i = 0; i < best.Indices.size(); ++i) put(best.Indices[i], i ? 4 : 3);
    if (cursor != 128) throw std::logic_error("Invalid BC7 block length");
    return result;
}

}
