#pragma once

#include "copy_to_swapchain.h"
#include "matrix.h"
#include "vk_utils.h"
#include "vk.h"

#include <vector>

struct GLFWwindow;

class Vk_Demo {
public:
    void initialize(GLFWwindow* glfw_window, bool enable_validation_layers);
    void shutdown();

    void release_resolution_dependent_resources();
    void restore_resolution_dependent_resources();
    bool vsync_enabled() const { return vsync; }
    void run_frame();

private:
    void create_depth_buffer();
    void destroy_depth_buffer();
    void draw_frame();
    void draw_rasterized_image();
    void draw_imgui();
    void copy_output_image_to_swapchain();
    void do_imgui();

private:
    using Clock = std::chrono::high_resolution_clock;
    using Time  = std::chrono::time_point<Clock>;

    bool show_ui = true;
    bool vsync = true;
    bool animate = false;

    Time last_frame_time;
    double sim_time;

    struct Depth_Buffer_Info {
        VkImage image;
        VkImageView image_view;
        VmaAllocation allocation;
    };
    Depth_Buffer_Info depth_info;

    GPU_Time_Keeper time_keeper;
    struct {
        GPU_Time_Interval* frame;
        GPU_Time_Interval* draw;
        GPU_Time_Interval* ui;
        GPU_Time_Interval* compute_copy;
    } gpu_times;

    VkRenderPass ui_render_pass;
    VkFramebuffer ui_framebuffer;
    VkRenderPass render_pass;
    VkFramebuffer framebuffer;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkDescriptorSet descriptor_set;
    Vk_Buffer uniform_buffer;
    void* mapped_uniform_buffer;
    Vk_Buffer vertex_buffer;
    Vk_Buffer index_buffer;
    uint32_t index_count;
    Vk_Image texture;
    VkSampler sampler;
    Vk_Image output_image;
    Copy_To_Swapchain copy_to_swapchain;

    Vector3 camera_pos = Vector3(0, 0.5, 3.0);
    Matrix3x4 model_transform;
    Matrix3x4 view_transform;
};
