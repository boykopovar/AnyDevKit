#version 450
layout(push_constant) uniform Parameters { vec2 offset; } parameters;
layout(location = 0) out vec3 color;
void main() {
    gl_Position = vec4(parameters.offset + vec2(float(gl_VertexIndex == 1), float(gl_VertexIndex == 2)), 0.0, 1.0);
    color = vec3(0.25, 0.5, 1.0);
}
