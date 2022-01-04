#pragma once

#include "linear_algebra.h"
#include "vk_utils.h"

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
    void do_imgui();

private:
    using Clock = std::chrono::high_resolution_clock;
    using Time  = std::chrono::time_point<Clock>;

    bool show_ui = true;
    bool vsync = true;
    bool animate = false;

	Time last_frame_time{};
    double sim_time = 0;
	Vector3 camera_pos = Vector3(0, 0.5, 3.0);

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
	} gpu_times{};

	VkRenderPass render_pass;
	std::vector<VkFramebuffer> framebuffers;

    VkRenderPass ui_render_pass;
    std::vector<VkFramebuffer> ui_framebuffers;

    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkDescriptorSet descriptor_set;
    Vk_Buffer uniform_buffer;
    void* mapped_uniform_buffer;
    GPU_Mesh gpu_mesh;
    Vk_Image texture;
    VkSampler sampler;
};
