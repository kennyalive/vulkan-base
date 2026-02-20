#version 460
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

layout(row_major) uniform;

layout(location=0) in vec2 frag_uv;
layout(location = 0) out vec4 color_attachment0;

layout(descriptor_heap) uniform texture2D images[];
layout(descriptor_heap) uniform sampler image_samplers[];

layout(push_constant) uniform PushData {
    uint buffer_descriptor_offset_index;
    uint image_descriptor_offset_index;
};

// Use this wrapper instead of using index indices directly until GLSL_EXT_structured_descriptor_heap is ready
uint GetImageDescriptorIndex(uint image_index) {
    return image_index + image_descriptor_offset_index;
}

void main() {
    const uint image_index = GetImageDescriptorIndex(0);
    color_attachment0 = texture(sampler2D(images[image_index], image_samplers[0]), frag_uv);
}
