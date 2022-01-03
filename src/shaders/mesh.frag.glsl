#version 460
layout(row_major) uniform;

layout(location=0) in vec2 frag_uv;
layout(location = 0) out vec4 color_attachment0;

layout(binding=1) uniform texture2D image;
layout(binding=2) uniform sampler image_sampler;

float srgb_encode(float c) {
    if (c <= 0.0031308f)
        return 12.92f * c;
    else
        return 1.055f * pow(c, 1.f/2.4f) - 0.055f;
}

vec3 srgb_encode(vec3 c) {
    return vec3(srgb_encode(c.r), srgb_encode(c.g), srgb_encode(c.b));
}

void main() {
    vec3 color = texture(sampler2D(image, image_sampler), frag_uv).xyz;
    color_attachment0 = vec4(srgb_encode(color), 1);
}
