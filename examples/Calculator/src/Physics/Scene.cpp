#include "Physics/Scene.hpp"
#include "CalculatorError.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace BackgroundPhysics {
namespace {

constexpr float HalfExtent = 0.55f;
constexpr float Radius = 0.64807407f;
constexpr float Step = 1.0f / 120.0f;
constexpr float Restitution = 0.94f;

Vector operator+(Vector a, Vector b) { return {a.X + b.X, a.Y + b.Y, a.Z + b.Z}; }
Vector operator-(Vector a, Vector b) { return {a.X - b.X, a.Y - b.Y, a.Z - b.Z}; }
Vector operator*(Vector a, float scale) { return {a.X * scale, a.Y * scale, a.Z * scale}; }
float dot(Vector a, Vector b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z; }
Vector cross(Vector a, Vector b) { return {a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X}; }

Vector normalized(Vector value) {
    const auto length = std::sqrt(dot(value, value));
    if (!(length > 0) || !std::isfinite(length)) throw CalculatorError{"Invalid physics direction"};
    return value * (1.0f / length);
}

float inverseMass(const Body& body) {
    return body.Sphere ? 1.0f / (4.1887902f * Radius * Radius * Radius) : 1.0f / (8.0f * HalfExtent * HalfExtent * HalfExtent);
}

float inverseInertia(const Body& body) {
    return inverseMass(body) / (body.Sphere ? 0.4f * Radius * Radius : 2.0f * HalfExtent * HalfExtent / 3.0f);
}

float extent(const Body& body, Vector axis) {
    if (body.Sphere) return Radius;
    return HalfExtent * (std::abs(dot(body.Axes[0], axis)) + std::abs(dot(body.Axes[1], axis)) + std::abs(dot(body.Axes[2], axis)));
}

Vector support(const Body& body, Vector direction) {
    if (body.Sphere) return body.Position + direction * Radius;
    auto point = body.Position;
    for (const auto axis : body.Axes) {
        const auto projection = dot(axis, direction);
        if (std::abs(projection) > 0.00001f) point = point + axis * std::copysign(HalfExtent, projection);
    }
    return point;
}

struct Contact {
    Vector Normal;
    Vector Point;
    float Depth = 0;
};

bool spheres(const Body& a, const Body& b, Contact& contact) {
    const auto delta = b.Position - a.Position;
    const auto squared = dot(delta, delta);
    if (squared >= 4 * Radius * Radius) return false;
    const auto distance = std::sqrt(squared);
    contact.Normal = distance > 0 ? delta * (1.0f / distance) : Vector{1, 0, 0};
    contact.Depth = 2 * Radius - distance;
    contact.Point = (a.Position + b.Position) * 0.5f;
    return true;
}

bool boxSphere(const Body& box, const Body& sphere, Contact& contact) {
    const auto delta = sphere.Position - box.Position;
    auto closest = box.Position;
    float nearestFace = std::numeric_limits<float>::max();
    Vector faceNormal;
    for (const auto axis : box.Axes) {
        const auto coordinate = dot(delta, axis);
        closest = closest + axis * std::clamp(coordinate, -HalfExtent, HalfExtent);
        const auto faceDistance = HalfExtent - std::abs(coordinate);
        if (faceDistance < nearestFace) {
            nearestFace = faceDistance;
            faceNormal = axis * (coordinate < 0 ? -1.0f : 1.0f);
        }
    }
    const auto offset = sphere.Position - closest;
    const auto squared = dot(offset, offset);
    if (squared >= Radius * Radius) return false;
    if (nearestFace >= 0) {
        contact.Normal = faceNormal;
        contact.Depth = Radius + nearestFace;
        contact.Point = sphere.Position + faceNormal * nearestFace;
    } else {
        const auto distance = std::sqrt(squared);
        contact.Normal = normalized(offset);
        contact.Depth = Radius - distance;
        contact.Point = closest;
    }
    return true;
}

bool boxes(const Body& a, const Body& b, Contact& contact) {
    const auto delta = b.Position - a.Position;
    contact.Depth = std::numeric_limits<float>::max();
    const auto separates = [&](Vector axis) {
        const auto squared = dot(axis, axis);
        if (squared < 0.00000001f) return false;
        axis = axis * (1.0f / std::sqrt(squared));
        const auto distance = dot(delta, axis);
        const auto depth = extent(a, axis) + extent(b, axis) - std::abs(distance);
        if (depth <= 0) return true;
        if (depth < contact.Depth) {
            contact.Depth = depth;
            contact.Normal = axis * (distance < 0 ? -1.0f : 1.0f);
        }
        return false;
    };
    for (const auto axis : a.Axes) if (separates(axis)) return false;
    for (const auto axis : b.Axes) if (separates(axis)) return false;
    for (const auto first : a.Axes) for (const auto second : b.Axes) if (separates(cross(first, second))) return false;
    contact.Point = (support(a, contact.Normal) + support(b, contact.Normal * -1)) * 0.5f;
    return true;
}

bool collide(const Body& a, const Body& b, Contact& contact) {
    if (a.Sphere && b.Sphere) return spheres(a, b, contact);
    if (!a.Sphere && !b.Sphere) return boxes(a, b, contact);
    if (!a.Sphere) return boxSphere(a, b, contact);
    if (!boxSphere(b, a, contact)) return false;
    contact.Normal = contact.Normal * -1;
    return true;
}

Vector pointVelocity(const Body& body, Vector arm) {
    return body.Velocity + cross(body.AngularVelocity, arm);
}

float effectiveMass(const Body& body, Vector arm, Vector direction) {
    const auto torque = cross(arm, direction);
    return inverseMass(body) + inverseInertia(body) * dot(torque, torque);
}

void impulse(Body& body, Vector arm, Vector value) {
    body.Velocity = body.Velocity + value * inverseMass(body);
    body.AngularVelocity = body.AngularVelocity + cross(arm, value) * inverseInertia(body);
}

void resolve(Body& a, Body& b, const Contact& contact) {
    const auto armA = contact.Point - a.Position;
    const auto armB = contact.Point - b.Position;
    const auto relative = pointVelocity(b, armB) - pointVelocity(a, armA);
    const auto speed = dot(relative, contact.Normal);
    if (speed < 0) {
        const auto magnitude = -(1 + Restitution) * speed / (effectiveMass(a, armA, contact.Normal) + effectiveMass(b, armB, contact.Normal));
        const auto normalImpulse = contact.Normal * magnitude;
        impulse(a, armA, normalImpulse * -1);
        impulse(b, armB, normalImpulse);
        const auto sliding = pointVelocity(b, armB) - pointVelocity(a, armA);
        const auto tangent = sliding - contact.Normal * dot(sliding, contact.Normal);
        const auto squared = dot(tangent, tangent);
        if (squared > 0.00000001f) {
            const auto direction = tangent * (1.0f / std::sqrt(squared));
            const auto friction = std::min(std::sqrt(squared) / (effectiveMass(a, armA, direction) + effectiveMass(b, armB, direction)), magnitude * 0.12f);
            impulse(a, armA, direction * friction);
            impulse(b, armB, direction * -friction);
        }
    }
    const auto correction = contact.Normal * (std::max(0.0f, contact.Depth - 0.0001f) * 0.8f / (inverseMass(a) + inverseMass(b)));
    a.Position = a.Position - correction * inverseMass(a);
    b.Position = b.Position + correction * inverseMass(b);
}

void wall(Body& body, Vector inward, float plane) {
    const auto depth = plane - dot(body.Position, inward) + extent(body, inward);
    if (depth <= 0) return;
    const auto arm = support(body, inward * -1) - body.Position;
    const auto speed = dot(pointVelocity(body, arm), inward);
    if (speed < 0) impulse(body, arm, inward * (-(1 + Restitution) * speed / effectiveMass(body, arm, inward)));
    body.Position = body.Position + inward * depth;
}

void rotate(Body& body) {
    const auto squared = dot(body.AngularVelocity, body.AngularVelocity);
    if (squared == 0) return;
    const auto halfRotation = body.AngularVelocity * (Step * 0.5f);
    const auto scale = 2.0f / (1 + dot(halfRotation, halfRotation));
    for (auto& basis : body.Axes) basis = basis + (cross(halfRotation, basis) + cross(halfRotation, cross(halfRotation, basis))) * scale;
    body.Axes[0] = normalized(body.Axes[0]);
    body.Axes[2] = normalized(cross(body.Axes[0], body.Axes[1]));
    body.Axes[1] = cross(body.Axes[2], body.Axes[0]);
}

}

Scene::Scene() {
    for (std::size_t index = 0; index < Count; ++index) {
        auto& body = bodies[index];
        const auto column = static_cast<float>(index % 4);
        const auto row = static_cast<float>(index / 4);
        body.Position = {-3.3f + column * 2.2f, -1.25f + row * 2.5f, 5.5f + 0.35f * static_cast<float>(index % 3)};
        body.Velocity = {(index % 2 == 0 ? 0.65f : -0.65f), (row == 0 ? 0.28f : -0.28f), (index % 3 == 0 ? 0.18f : -0.12f)};
        body.AngularVelocity = {0.12f + column * 0.04f, 0.2f - row * 0.08f, 0.08f};
        body.Sphere = (index & 1) != 0;
    }
}

void Scene::Update(float seconds) {
    if (!std::isfinite(seconds) || seconds < previousSeconds) throw CalculatorError{"Invalid physics time"};
    const auto elapsed = seconds - previousSeconds;
    previousSeconds = seconds;
    accumulator += std::min(elapsed, 0.1f);
    while (accumulator >= Step) {
        step();
        accumulator -= Step;
    }
}

void Scene::step() {
    for (auto& body : bodies) {
        body.Position = body.Position + body.Velocity * Step;
        rotate(body);
    }
    for (int iteration = 0; iteration < 6; ++iteration) {
        for (std::size_t first = 0; first < Count; ++first) {
            for (std::size_t second = first + 1; second < Count; ++second) {
                Contact contact;
                if (collide(bodies[first], bodies[second], contact)) resolve(bodies[first], bodies[second], contact);
            }
        }
        for (auto& body : bodies) {
            wall(body, {1, 0, 0}, -4.5f);
            wall(body, {-1, 0, 0}, -4.5f);
            wall(body, {0, 1, 0}, -2.8f);
            wall(body, {0, -1, 0}, -2.8f);
            wall(body, {0, 0, 1}, 3.8f);
            wall(body, {0, 0, -1}, -7.5f);
        }
    }
}

void Scene::WriteTransforms(std::span<float, TransformFloats> output) const {
    for (std::size_t index = 0; index < Count; ++index) {
        const auto& body = bodies[index];
        output[index * 4] = body.Position.X;
        output[index * 4 + 1] = body.Position.Y;
        output[index * 4 + 2] = body.Position.Z;
        output[index * 4 + 3] = 0;
        if (body.Sphere) continue;
        const auto& x = body.Axes[0];
        const auto& y = body.Axes[1];
        const auto& z = body.Axes[2];
        std::array<float, 4> quaternion;
        const auto trace = x.X + y.Y + z.Z;
        if (trace > 0) {
            const auto scale = 2 * std::sqrt(1 + trace);
            quaternion = {(y.Z - z.Y) / scale, (z.X - x.Z) / scale, (x.Y - y.X) / scale, scale * 0.25f};
        } else if (x.X > y.Y && x.X > z.Z) {
            const auto scale = 2 * std::sqrt(1 + x.X - y.Y - z.Z);
            quaternion = {scale * 0.25f, (y.X + x.Y) / scale, (z.X + x.Z) / scale, (y.Z - z.Y) / scale};
        } else if (y.Y > z.Z) {
            const auto scale = 2 * std::sqrt(1 + y.Y - x.X - z.Z);
            quaternion = {(y.X + x.Y) / scale, scale * 0.25f, (z.Y + y.Z) / scale, (z.X - x.Z) / scale};
        } else {
            const auto scale = 2 * std::sqrt(1 + z.Z - x.X - y.Y);
            quaternion = {(z.X + x.Z) / scale, (z.Y + y.Z) / scale, scale * 0.25f, (x.Y - y.X) / scale};
        }
        for (std::size_t component = 0; component < 4; ++component) output[Count * 4 + index / 2 * 4 + component] = quaternion[component];
    }
}

}
