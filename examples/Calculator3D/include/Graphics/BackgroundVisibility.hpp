#ifndef ANYDEVKIT_CALCULATOR3D_GRAPHICS_BACKGROUNDVISIBILITY_HPP
#define ANYDEVKIT_CALCULATOR3D_GRAPHICS_BACKGROUNDVISIBILITY_HPP

#include "Graphics/DrawCommand.hpp"
#include <span>
#include <vector>

class BackgroundVisibility final {
public:
    std::span<const std::array<float, 4>> Update(int width, int height, std::span<const DrawCommand> commands);

private:
    void subtract(const std::array<float, 4>& rectangle);
    std::vector<std::array<float, 4>> regions;
    std::vector<std::array<float, 4>> scratch;
};

#endif

