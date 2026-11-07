#version 450

layout(set = 0, binding = 1, std430) readonly buffer Frame {
    vec2 resolution;
    float seconds;
    vec4 positions[8];
    vec4 orientations[4];
    vec4 formulaPositions[8];
    vec4 formulaOrientations[8];
} frame;

layout(set = 0, binding = 0, std430) readonly buffer Regions {
    vec4 values[];
} regions;

layout(location = 0) flat out vec2 resolution;
layout(location = 1) flat out vec4 positions[8];
layout(location = 9) flat out vec4 orientations[4];
layout(location = 13) flat out vec4 textCenters[8];
layout(location = 21) flat out vec4 textRotations[8];

layout(location = 29) flat out uint visibility;

bool overlaps(vec4 rectangle, vec3 center, float radius) {
    if (center.z <= radius) return true;
    float aspect = frame.resolution.x / frame.resolution.y;
    vec2 slope = center.xy / center.z;
    vec2 projected = slope * vec2(2.3 / aspect, -2.3);
    vec2 extent = 2.3 * radius * (vec2(1.0) + abs(slope)) / (center.z - radius);
    vec2 pixelCenter = (projected * 0.5 + 0.5) * frame.resolution;
    vec2 pixelExtent = vec2(extent.x / aspect, extent.y) * 0.5 * frame.resolution + 2.0;

    return all(lessThan(rectangle.xy, pixelCenter + pixelExtent)) && all(greaterThan(rectangle.zw, pixelCenter - pixelExtent));
}

void main() {
    const vec2 corners[6] = vec2[6](vec2(0, 0), vec2(1, 0), vec2(0, 1), vec2(0, 1), vec2(1, 0), vec2(1, 1));
    vec4 rectangle = regions.values[gl_VertexIndex / 6];
    vec2 position = mix(rectangle.xy, rectangle.zw, corners[gl_VertexIndex % 6]) / frame.resolution;
    gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
    resolution = frame.resolution;
    visibility = 0u;

    for (int index = 0; index < 8; ++index) {
        if (overlaps(rectangle, frame.positions[index].xyz, (index & 1) == 1 ? sqrt(0.42) : sqrt(0.91))) visibility |= 1u << index;
        float radius = sqrt(frame.formulaPositions[index].w * frame.formulaPositions[index].w + (7.0 / 45.0) * (7.0 / 45.0) + 0.001);
        if (overlaps(rectangle, frame.formulaPositions[index].xyz, radius)) visibility |= 1u << (index + 8);
        positions[index] = frame.positions[index];
        textCenters[index] = frame.formulaPositions[index];
        textRotations[index] = frame.formulaOrientations[index];
    }

    for (int index = 0; index < 4; ++index) orientations[index] = frame.orientations[index];
}

