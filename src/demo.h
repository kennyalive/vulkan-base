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

    Vk_GPU_Time_Keeper time_keeper;
    struct {
        Vk_GPU_Time_Interval* frame;
    } gpu_times{};

    Vk_Image depth_buffer_image;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    Vk_Buffer descriptor_buffer;
    void* mapped_descriptor_buffer_ptr = nullptr;
    Vk_Buffer uniform_buffer;
    void* mapped_uniform_buffer = nullptr;
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
