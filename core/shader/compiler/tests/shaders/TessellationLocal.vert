#version 450
layout(push_constant) uniform Parameters { float scale; } parameters;
layout(location = 0) out vec3 color;
void main() {
    gl_Position = vec4(vec2(float(gl_VertexIndex == 1), float(gl_VertexIndex == 2)) * parameters.scale, 0.0, 1.0);
    color = vec3(1.0, 0.5, 0.25);
}
