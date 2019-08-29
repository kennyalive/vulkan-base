layout(row_major) uniform;

struct Frag_In {
    vec3 normal;
    vec2 uv;
};

float srgb_encode(float c) {
    if (c <= 0.0031308f)
        return 12.92f * c;
    else
        return 1.055f * pow(c, 1.f/2.4f) - 0.055f;
}

vec3 srgb_encode(vec3 c) {
    return vec3(srgb_encode(c.r), srgb_encode(c.g), srgb_encode(c.b));
}

vec3 color_encode_lod(float lod) {
    uint color_mask = (uint(floor(lod)) + 1) & 7;
    vec3 color0 = vec3(float(color_mask&1), float(color_mask&2), float(color_mask&4));
    vec3 color1 = 0.25 * color0;
    return mix(color0, color1, fract(lod));
}

