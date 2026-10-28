#version 450
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
layout(location = 0) in vec3 inputColor[];
layout(location = 0) out vec3 color;
void main() {
    for (int index = 0; index < 3; ++index) {
        gl_Position = gl_in[index].gl_Position;
        color = inputColor[index];
        EmitVertex();
    }
    EndPrimitive();
}
