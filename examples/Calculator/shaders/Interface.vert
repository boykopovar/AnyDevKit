#version 450

layout(push_constant) uniform Drawing {
    vec4 rectangle;
    vec4 color;
    vec4 parameters;
    uvec4 glyph;
} drawing;

void main() {
    const vec2 corners[6] = vec2[6](vec2(0, 0), vec2(1, 0), vec2(0, 1), vec2(0, 1), vec2(1, 0), vec2(1, 1));
    vec2 position = drawing.rectangle.xy + corners[gl_VertexIndex] * drawing.rectangle.zw;
    gl_Position = vec4(position / drawing.parameters.yz * 2.0 - 1.0, 0.0, 1.0);
}
