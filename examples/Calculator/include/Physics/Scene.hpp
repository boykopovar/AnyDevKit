#ifndef ANYDEVKIT_CALCULATOR_PHYSICS_SCENE_HPP
#define ANYDEVKIT_CALCULATOR_PHYSICS_SCENE_HPP

#include <array>
#include <span>

namespace BackgroundPhysics {

struct Vector {
    float X = 0;
    float Y = 0;
    float Z = 0;
};

struct Body {
    Vector Position;
    Vector Velocity;
    Vector AngularVelocity;
    std::array<Vector, 3> Axes{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    bool Sphere = false;
};

class Scene final {
public:
    static constexpr std::size_t Count = 8;
    static constexpr std::size_t TransformFloats = Count * 4 + Count / 2 * 4;
    Scene();
    void Update(float seconds);
    void WriteTransforms(std::span<float, TransformFloats> output) const;

private:
    void step();
    std::array<Body, Count> bodies;
    float previousSeconds = 0;
    double accumulator = 0;
};

}

#endif
