#version 450

struct Drawing {
    vec4 rectangle;
    vec4 color;
    vec4 parameters;
    uvec4 glyph;
};

layout(set = 0, binding = 0, std430) readonly buffer Drawings {
    Drawing values[];
} drawings;

layout(location = 0) out vec2 localPosition;
layout(location = 1) flat out vec4 rectangle;
layout(location = 2) flat out vec4 tint;
layout(location = 3) flat out vec4 parameters;
layout(location = 4) flat out uvec4 glyph;

void main() {
    const vec2 corners[6] = vec2[6](vec2(0, 0), vec2(1, 0), vec2(0, 1), vec2(0, 1), vec2(1, 0), vec2(1, 1));
    Drawing drawing = drawings.values[gl_VertexIndex / 6];
    rectangle = drawing.rectangle;
    tint = drawing.color;
    parameters = drawing.parameters;
    glyph = drawing.glyph;
    localPosition = corners[gl_VertexIndex % 6] * rectangle.zw;
    vec2 position = drawing.rectangle.xy + localPosition;
    gl_Position = vec4(position / drawing.parameters.yz * 2.0 - 1.0, 0.0, 1.0);
}

