#version 460
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

layout(row_major) uniform;

layout(location=0) in vec4 in_position;
layout(location=1) in vec2 in_uv;
layout(location = 0) out vec2 frag_uv;

// TODO: this uniform block should contain just mat4x4.
// There is a bug in nvidia beta vulkan drivers (582.32) that causes crash when mat4x4 is used with descriptor heaps 
layout(descriptor_heap) uniform Uniform_Block {
    vec4 row0;
    vec4 row1;
    vec4 row2;
    vec4 row3;
} buffers[];

// NOTE: the heap_offset in bytes hopefully will be added in the future extension
// (https://github.com/KhronosGroup/GLSL/pull/299) and it will allow to specify,
// for example, that buffer descriptors start at this offset and image descriptors
// start at another offset. Until it's ready, we will use push data to specify index
// offsets (not bytes) and use wrapper functions that compute final index.
layout(push_constant) uniform PushData {
    uint buffer_descriptor_index_offset;
    uint image_descriptor_index_offset;
};

// Use this wrapper instead of using buffer indices directly until GLSL_EXT_structured_descriptor_heap is ready
uint GetBufferDescriptorIndex(uint buffer_index) {
    return buffer_index + buffer_descriptor_index_offset;
}

void main() {
    frag_uv = in_uv;

    const uint buffer_index = GetBufferDescriptorIndex(0);

    // TODO: when mat4x4 driver bug is fixed use this: buffers[heap_index].transform
    vec4 r0 = buffers[buffer_index].row0;
    vec4 r1 = buffers[buffer_index].row1;
    vec4 r2 = buffers[buffer_index].row2;
    vec4 r3 = buffers[buffer_index].row3;
    mat4x4 transform = transpose(mat4x4(r0, r1, r2, r3));

    gl_Position = transform * in_position;
}
