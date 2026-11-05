#version 450

layout(push_constant) uniform Frame {
    vec2 resolution;
    float seconds;
    vec4 positions[8];
    vec4 orientations[4];
} frame;

layout(location = 0) flat out vec2 resolution;
layout(location = 1) flat out vec4 positions[8];
layout(location = 9) flat out vec4 orientations[4];
layout(location = 13) flat out vec4 textCenters[8];
layout(location = 21) flat out vec4 textRotations[8];

void main() {
    vec2 position = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
    resolution = frame.resolution;
    for (int index = 0; index < 8; ++index) {
        positions[index] = frame.positions[index];
        float phase = float(index) * 1.73;
        float angle = frame.seconds * 0.12 + phase;
        textCenters[index] = vec4(vec3(sin(angle) * 4.4, cos(angle * 0.7 + phase) * 2.8, 5.0 + 1.5 * sin(angle * 0.6)) + vec3(-1.0, 0.9, -0.8), 0.0);
        textRotations[index] = vec4(cos(angle), sin(angle), cos(angle * 0.7), sin(angle * 0.7));
    }
    for (int index = 0; index < 4; ++index) orientations[index] = frame.orientations[index];
}
