#pragma once

#include "lib.h"
#include "vk.h"
#include <chrono>

struct GLFWwindow;

class Vk_Demo {
public:
    void initialize(GLFWwindow* glfw_window);
    void shutdown();
    void release_resolution_dependent_resources();
    void restore_resolution_dependent_resources();
    bool vsync_enabled() const { return vsync; }
    void run_frame();

private:
    void do_imgui();
    void draw_frame();

private:
    using Clock = std::chrono::high_resolution_clock;
    using Time  = std::chrono::time_point<Clock>;

    bool show_ui = true;
    bool vsync = true;
    bool animate = false;

    Time last_frame_time{};
    double sim_time = 0;
    Vector3 camera_pos = Vector3(0, 0.5, 3.0);

    Vk_Time_Keeper time_keeper;
    Vk_Timer* frame_timer = nullptr;

    Vk_Image depth_buffer_image;
    VkPipeline pipeline;

    Vk_Buffer resource_descriptor_heap;
    VkDeviceSize resource_descriptor_heap_size = 0;
    VkDeviceSize resource_reserved_offset = 0;
    VkDeviceSize resource_reserved_size = 0;

    // TODO: Specify index offset until GLSL_EXT_structured_descriptor_heap is ready.
    // When it's ready this should be replaced with byte offsets.
    uint32_t buffer_descriptor_index_offset = 0;
    uint32_t image_descriptor_index_offset = 0;

    Vk_Buffer sampler_descriptor_heap;
    VkDeviceSize sampler_descriptor_heap_size = 0;
    VkDeviceSize sampler_reserved_offset = 0;
    VkDeviceSize sampler_reserved_size = 0;

    Vk_Buffer uniform_buffer;
    Vk_Image texture;
    VkSampler sampler;

    struct {
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
    } gpu_mesh;
};
