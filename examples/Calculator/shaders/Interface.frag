#version 450

layout(push_constant) uniform Drawing {
    vec4 rectangle;
    vec4 color;
    vec4 parameters;
    uvec4 glyph;
} drawing;

layout(location = 0) out vec4 outputColor;

void main() {
    vec2 position = gl_FragCoord.xy - drawing.rectangle.xy;
    float scale = drawing.parameters.w;
    if (scale > 0.0) {
        uvec2 cell = uvec2(floor(position / scale));
        if (cell.x >= 5u || cell.y >= 7u) discard;
        uint row = drawing.glyph[cell.y / 4u] >> ((cell.y % 4u) * 5u);
        if ((row & (1u << (4u - cell.x))) == 0u) discard;
    } else {
        float radius = drawing.parameters.x;
        vec2 corner = max(abs(position - drawing.rectangle.zw * 0.5) - (drawing.rectangle.zw * 0.5 - radius), vec2(0.0));
        if (dot(corner, corner) > radius * radius) discard;
    }
    outputColor = drawing.color;
}
