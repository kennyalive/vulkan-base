#pragma once

#include "lib.h"
#include "vk.h"

struct GPU_Mesh {
    Vk_Buffer vertex_buffer;
    Vk_Buffer index_buffer;
    uint32_t vertex_count = 0;
    uint32_t index_count = 0;

    void destroy() {
        vertex_buffer.destroy();
        index_buffer.destroy();
        vertex_count = 0;
        index_count = 0;
    }
};

struct Shader_Module {
    Shader_Module(const std::string& spirv_file) {
        handle = vk_load_spirv((get_data_directory() / spirv_file).string());
    }
    ~Shader_Module() {
        vkDestroyShaderModule(vk.device, handle, nullptr);
    }
    VkShaderModule handle;
};

struct Descriptor_Set_Layout {
    static constexpr uint32_t max_bindings = 32;

    VkDescriptorSetLayoutBinding bindings[max_bindings];
    uint32_t binding_count;

    Descriptor_Set_Layout() {
        binding_count = 0;
    }

    Descriptor_Set_Layout& sampled_image(uint32_t binding, VkShaderStageFlags stage_flags);
    Descriptor_Set_Layout& storage_image(uint32_t binding, VkShaderStageFlags stage_flags);
    Descriptor_Set_Layout& sampler(uint32_t binding, VkShaderStageFlags stage_flags);
    Descriptor_Set_Layout& uniform_buffer(uint32_t binding, VkShaderStageFlags stage_flags);
    Descriptor_Set_Layout& storage_buffer(uint32_t binding, VkShaderStageFlags stage_flags);
    Descriptor_Set_Layout& accelerator(uint32_t binding, VkShaderStageFlags stage_flags);
    VkDescriptorSetLayout create(const char* name);
};

//
// GPU time queries.
//
struct GPU_Time_Interval {
    uint32_t start_query[2]; // end query == (start_query[frame_index] + 1)
    float length_ms;

    void begin();
    void end();
};

struct GPU_Time_Keeper {
    static constexpr uint32_t max_time_intervals = 128;

    GPU_Time_Interval time_intervals[max_time_intervals];
    uint32_t time_interval_count;

    GPU_Time_Interval* allocate_time_interval();
    void initialize_time_intervals();
    void next_frame();
};

struct GPU_Time_Scope {
    GPU_Time_Scope(GPU_Time_Interval* time_interval) {
        this->time_interval = time_interval;
        time_interval->begin();
    }
    ~GPU_Time_Scope() {
        time_interval->end();
    }

private:
    GPU_Time_Interval* time_interval;
};

#define GPU_TIME_SCOPE(time_interval) GPU_Time_Scope gpu_time_scope##__LINE__(time_interval)

//
// GPU debug markers.
//
void begin_gpu_marker_scope(VkCommandBuffer command_buffer, const char* name);
void end_gpu_marker_scope(VkCommandBuffer command_buffer);
void write_gpu_marker(VkCommandBuffer command_buffer, const char* name);

struct GPU_Marker_Scope {
    GPU_Marker_Scope(VkCommandBuffer command_buffer, const char* name) {
        this->command_buffer = command_buffer;
        begin_gpu_marker_scope(command_buffer, name);
    }
    ~GPU_Marker_Scope() {
        end_gpu_marker_scope(command_buffer);
    }

private:
    VkCommandBuffer command_buffer;
};

#define GPU_MARKER_SCOPE(command_buffer, name) GPU_Marker_Scope gpu_marker_scope##__LINE__(command_buffer, name)

