#include "Graphics/BackgroundVisibility.hpp"
#include <algorithm>

void BackgroundVisibility::subtract(const std::array<float, 4>& rectangle) {
    scratch.clear();
    for (const auto& region : regions) {
        const float left = std::max(region[0], rectangle[0]);
        const float top = std::max(region[1], rectangle[1]);
        const float right = std::min(region[2], rectangle[2]);
        const float bottom = std::min(region[3], rectangle[3]);
        if (left >= right || top >= bottom) {
            scratch.push_back(region);
            continue;
        }
        if (region[1] < top) scratch.push_back({region[0], region[1], region[2], top});
        if (bottom < region[3]) scratch.push_back({region[0], bottom, region[2], region[3]});
        if (region[0] < left) scratch.push_back({region[0], top, left, bottom});
        if (right < region[2]) scratch.push_back({right, top, region[2], bottom});
    }
    regions.swap(scratch);
}

std::span<const std::array<float, 4>> BackgroundVisibility::Update(int width, int height, std::span<const DrawCommand> commands) {
    regions.clear();
    regions.push_back({0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)});
    for (const auto& command : commands) {
        if (command.Color[3] != 1.0f || command.Parameters[3] != 0.0f) continue;
        const auto& rectangle = command.Rectangle;
        const float radius = command.Parameters[0];
        subtract({rectangle[0] + radius, rectangle[1], rectangle[0] + rectangle[2] - radius, rectangle[1] + rectangle[3]});
        if (radius > 0.0f) subtract({rectangle[0], rectangle[1] + radius, rectangle[0] + rectangle[2], rectangle[1] + rectangle[3] - radius});
    }
    return regions;
}
