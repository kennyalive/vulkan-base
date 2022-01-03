#version 460
layout(row_major) uniform;

layout(location=0) in vec4 in_position;
layout(location=1) in vec2 in_uv;
layout(location = 0) out vec2 frag_uv;

layout(std140, binding=0) uniform Uniform_Block {
    mat4x4 model_view_proj;
};

void main() {
    frag_uv = in_uv;
    gl_Position = model_view_proj * in_position;
}
