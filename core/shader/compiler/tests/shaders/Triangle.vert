#version 450
layout(location = 0) out vec3 color;
void main() {
    const vec2 positions[3] = vec2[3](vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(0.0, 0.5));
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    color = vec3(float(gl_VertexIndex == 0), float(gl_VertexIndex == 1), float(gl_VertexIndex == 2));
}
