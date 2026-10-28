#version 450
layout(triangles, equal_spacing, ccw) in;
layout(location = 0) in vec3 inputColor[];
layout(location = 0) out vec3 color;
void main() {
    gl_Position = gl_TessCoord.x * gl_in[0].gl_Position + gl_TessCoord.y * gl_in[1].gl_Position + gl_TessCoord.z * gl_in[2].gl_Position;
    color = gl_TessCoord.x * inputColor[0] + gl_TessCoord.y * inputColor[1] + gl_TessCoord.z * inputColor[2];
}
