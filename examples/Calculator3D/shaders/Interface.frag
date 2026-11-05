#version 450

layout(location = 0) in vec2 localPosition;
layout(location = 1) flat in vec4 rectangle;
layout(location = 2) flat in vec4 tint;
layout(location = 3) flat in vec4 parameters;
layout(location = 4) flat in uvec4 glyph;

layout(location = 0) out vec4 outputColor;

void main() {
    vec2 position = localPosition;
    float scale = parameters.w;
    if (scale > 0.0) {
        uvec2 cell = uvec2(floor(position / scale));
        if (cell.x >= 5u || cell.y >= 7u) discard;
        uint packedRows = cell.y < 4u ? glyph.x : glyph.y;
        uint row = packedRows >> ((cell.y % 4u) * 5u);
        if ((row & (1u << (4u - cell.x))) == 0u) discard;
    } else {
        float radius = parameters.x;
        vec2 corner = max(abs(position - rectangle.zw * 0.5) - (rectangle.zw * 0.5 - radius), vec2(0.0));
        if (dot(corner, corner) > radius * radius) discard;
    }
    outputColor = tint;
}
