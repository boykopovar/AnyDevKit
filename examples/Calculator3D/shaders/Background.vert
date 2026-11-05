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

void main() {
    const vec2 corners[6] = vec2[6](vec2(0, 0), vec2(1, 0), vec2(0, 1), vec2(0, 1), vec2(1, 0), vec2(1, 1));
    vec4 rectangle = regions.values[gl_VertexIndex / 6];
    vec2 position = mix(rectangle.xy, rectangle.zw, corners[gl_VertexIndex % 6]) / frame.resolution;
    gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
    resolution = frame.resolution;
    for (int index = 0; index < 8; ++index) {
        positions[index] = frame.positions[index];
        textCenters[index] = frame.formulaPositions[index];
        textRotations[index] = frame.formulaOrientations[index];
    }
    for (int index = 0; index < 4; ++index) orientations[index] = frame.orientations[index];
}
