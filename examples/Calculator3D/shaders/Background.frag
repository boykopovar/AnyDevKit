#version 450

layout(location = 0) out vec4 color;
layout(location = 0) flat in vec2 resolution;
layout(location = 1) flat in vec4 positions[8];
layout(location = 9) flat in vec4 orientations[4];
layout(location = 13) flat in vec4 textCenters[8];
layout(location = 21) flat in vec4 textRotations[8];
layout(location = 29) flat in uint visibility;

mat3 orientation(vec4 q) {
    return mat3(
        1.0 - 2.0 * (q.y * q.y + q.z * q.z), 2.0 * (q.x * q.y + q.z * q.w), 2.0 * (q.x * q.z - q.y * q.w),
        2.0 * (q.x * q.y - q.z * q.w), 1.0 - 2.0 * (q.x * q.x + q.z * q.z), 2.0 * (q.y * q.z + q.x * q.w),
        2.0 * (q.x * q.z + q.y * q.w), 2.0 * (q.y * q.z - q.x * q.w), 1.0 - 2.0 * (q.x * q.x + q.y * q.y));
}

float cube(vec3 origin, vec3 direction, out vec3 normal) {
    vec3 inverse = 1.0 / direction;
    vec3 a = (-vec3(0.55) - origin) * inverse;
    vec3 b = (vec3(0.55) - origin) * inverse;
    vec3 nearPlane = min(a, b);
    vec3 farPlane = max(a, b);
    float nearDistance = max(max(nearPlane.x, nearPlane.y), nearPlane.z);
    float farDistance = min(min(farPlane.x, farPlane.y), farPlane.z);
    normal = -sign(direction) * step(nearPlane.yzx, nearPlane) * step(nearPlane.zxy, nearPlane);

    return nearDistance > 0.0 && farDistance >= nearDistance ? nearDistance : 1000.0;
}

uint glyph(int character, int row) {
    const uint x[7] = uint[7](17, 17, 10, 4, 10, 17, 17);
    const uint y[7] = uint[7](17, 17, 10, 4, 4, 4, 4);
    const uint z[7] = uint[7](31, 1, 2, 4, 8, 16, 31);
    const uint two[7] = uint[7](14, 17, 1, 2, 4, 8, 31);
    const uint plus[7] = uint[7](0, 4, 4, 31, 4, 4, 0);
    const uint equals[7] = uint[7](0, 0, 31, 0, 31, 0, 0);
    const uint one[7] = uint[7](4, 12, 4, 4, 4, 4, 14);
    if (character == 0) return x[row];
    if (character == 1) return y[row];
    if (character == 2) return z[row];
    if (character == 3) return two[row];
    if (character == 4) return plus[row];
    if (character == 5) return equals[row];
    return one[row];
}

float formula(vec2 uv, int variation) {
    ivec2 pixel = ivec2(floor(uv * vec2(54.0, 9.0)));
    if (pixel.x < 0 || pixel.x >= 54 || pixel.y < 0 || pixel.y >= 7) return 0.0;
    int column = pixel.x % 6;
    if (column == 5) return 0.0;
    int index = pixel.x / 6;
    const int equation[9] = int[9](0, 3, 4, 1, 3, 5, 2, 3, 6);
    int character = equation[index];
    if (index == 8) return 0.0;
    if (variation == 1 && index == 6) character = 6;
    if (variation == 1 && index == 7) return 0.0;
    return float((glyph(character, pixel.y) >> uint(4 - column)) & 1u);
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;
    vec2 screen = (uv * 2.0 - 1.0) * vec2(resolution.x / resolution.y, -1.0);
    vec3 direction = normalize(vec3(screen, 2.3));
    vec3 background = mix(vec3(0.025, 0.050, 0.080), vec3(0.045, 0.085, 0.12), uv.y);
    background += 0.012 * pow(max(0.0, 1.0 - length(screen * 0.55)), 2.0);
    if (visibility == 0u) {
        color = vec4(background, 1.0);
        return;
    }

    float closest = 1000.0;
    vec3 surface = background;
    for (int index = 0; index < 8; ++index) {
        if ((visibility & (1u << index)) != 0u) {
            vec3 center = positions[index].xyz;
            vec3 normal;
            float distance = 1000.0;
            float b = dot(-center, direction);
            float discriminant = b * b - dot(center, center) + ((index & 1) == 1 ? 0.42 : 0.91);
            if (discriminant > 0.0) {
                if ((index & 1) == 1) {
                    distance = -b - sqrt(discriminant);
                    normal = normalize(direction * distance - center);
                } else {
                    mat3 basis = orientation(orientations[index / 2]);
                    vec3 origin = transpose(basis) * -center;
                    vec3 ray = transpose(basis) * direction;
                    distance = cube(origin, ray, normal);
                    normal = basis * normal;
                }
            }

            if (distance > 0.0 && distance < closest) {
                closest = distance;
                float light = 0.25 + 0.75 * max(0.0, dot(normal, normalize(vec3(-0.4, 0.8, -1.0))));
                vec3 tint = (index & 1) == 0 ? vec3(0.07, 0.22, 0.24) : vec3(0.15, 0.12, 0.28);
                surface = tint * light + vec3(0.018, 0.025, 0.035);
            }
        }

        if ((visibility & (1u << (index + 8))) != 0u) {
            vec3 formulaCenter = textCenters[index].xyz;
            float formulaProjection = dot(formulaCenter, direction);
            float formulaRadiusSquared = textCenters[index].w * textCenters[index].w + (7.0 / 45.0) * (7.0 / 45.0) + 0.001;
            if (formulaProjection * formulaProjection - dot(formulaCenter, formulaCenter) + formulaRadiusSquared > 0.0) {
                mat3 basis = orientation(textRotations[index]);
                vec3 textCenter = textCenters[index].xyz - basis[0] * textCenters[index].w + basis[1] * (7.0 / 45.0);
                vec3 textNormal = basis[2];
                float denominator = dot(direction, textNormal);
                if (abs(denominator) > 0.001) {
                    float planeDistance = dot(textCenter, textNormal) / denominator;
                    vec3 local = transpose(basis) * (direction * planeDistance - textCenter);
                    vec2 textUv = vec2(local.x / 2.0, -local.y / 0.4);
                    if (planeDistance > 0.0 && planeDistance < closest && formula(textUv, index & 1) > 0.0) {
                        closest = planeDistance;
                        surface = vec3(0.13, 0.34, 0.32);
                    }
                }
            }
        }
    }

    color = vec4(surface, 1.0);
}

