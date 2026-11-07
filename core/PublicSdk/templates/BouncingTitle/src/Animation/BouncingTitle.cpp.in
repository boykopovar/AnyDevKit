#include "Animation/BouncingTitle.hpp"
#include "Error.hpp"
#include <algorithm>

BouncingTitle::BouncingTitle(std::string_view text, int width, int height) {
    if (text.empty() || text.size() > 64 || width < 512 || height < 256) throw Error{"Invalid title extent"};
    scale = static_cast<float>(std::min(10, (width - 96) / static_cast<int>(text.size() * 6)));
    if (scale < 1) throw Error{"Title does not fit the display"};
    bounds = {width - (text.size() * 6 - 1) * scale, height - 7 * scale};
}

void BouncingTitle::Update(double seconds) {
    if (!(seconds >= 0 && seconds < 60)) throw Error{"Invalid animation interval"};
    for (std::size_t axis = 0; axis < position.size(); ++axis) {
        position[axis] += velocity[axis] * seconds;

        while (position[axis] < 0 || position[axis] > bounds[axis]) {
            if (position[axis] < 0) position[axis] = -position[axis];
            else position[axis] = 2 * bounds[axis] - position[axis];
            velocity[axis] = -velocity[axis];
        }
    }
}

std::array<float, 2> BouncingTitle::Position() const { return {static_cast<float>(position[0]), static_cast<float>(position[1])}; }
float BouncingTitle::Scale() const { return scale; }

