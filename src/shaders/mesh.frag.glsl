#version 460
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout(location=0) in Frag_In frag_in;
layout(location = 0) out vec4 color_attachment0;

layout(binding=1) uniform texture2D image;
layout(binding=2) uniform sampler image_sampler;

void main() {
    vec3 color = texture(sampler2D(image, image_sampler), frag_in.uv).xyz;
    color_attachment0 = vec4(srgb_encode(color), 1);
}
