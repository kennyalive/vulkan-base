#include "demo.h"

#include "glfw/glfw3.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_glfw.h"

#include <array>

static VkFormat get_depth_image_format() {
    VkFormat candidates[2] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_X8_D24_UNORM_PACK32 };
    for (auto format : candidates) {
        VkFormatProperties props{};
        vkGetPhysicalDeviceFormatProperties(vk.physical_device, format, &props);
        if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) {
            return format;
        }
    }
    error("failed to select depth attachment format");
    return VK_FORMAT_UNDEFINED;
}

void Vk_Demo::initialize(GLFWwindow* window) {
    Vk_Init_Params vk_init_params;
    vk_init_params.error_reporter = &error;

    std::array instance_extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
    };
    std::array device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
    };
    vk_init_params.instance_extensions = std::span{ instance_extensions };
    vk_init_params.device_extensions = std::span{ device_extensions };

    // Specify required features.
    VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    Vk_PNexer pnexer(features2);
    vk_init_params.device_create_info_pnext = (const VkBaseInStructure*)&features2;

    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
    buffer_device_address_features.bufferDeviceAddress = VK_TRUE;
    pnexer.next(buffer_device_address_features);

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
    dynamic_rendering_features.dynamicRendering = VK_TRUE;
    pnexer.next(dynamic_rendering_features);

    VkPhysicalDeviceSynchronization2Features synchronization2_features{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES };
    synchronization2_features.synchronization2 = VK_TRUE;
    pnexer.next(synchronization2_features);

    VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptor_buffer_features{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT };
    descriptor_buffer_features.descriptorBuffer = VK_TRUE;
    pnexer.next(descriptor_buffer_features);

    // Surface formats
    std::array surface_formats = {
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R8G8B8A8_SRGB,
    };
    vk_init_params.supported_surface_formats = std::span{ surface_formats };
    vk_init_params.surface_usage_flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    vk_initialize(window, vk_init_params);

    // Device properties.
    {
        VkPhysicalDeviceProperties2 physical_device_properties { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        vkGetPhysicalDeviceProperties2(vk.physical_device, &physical_device_properties);
        printf("Device: %s\n", physical_device_properties.properties.deviceName);
        printf("Vulkan API version: %d.%d.%d\n",
            VK_VERSION_MAJOR(physical_device_properties.properties.apiVersion),
            VK_VERSION_MINOR(physical_device_properties.properties.apiVersion),
            VK_VERSION_PATCH(physical_device_properties.properties.apiVersion)
        );
    }

    // Geometry buffers.
    {
        Triangle_Mesh mesh = load_obj_model(get_resource_path("model/mesh.obj"), 1.25f);
        {
            const VkDeviceSize size = mesh.vertices.size() * sizeof(mesh.vertices[0]);
            VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            gpu_mesh.vertex_buffer = vk_create_buffer(size, usage, mesh.vertices.data(), "vertex_buffer");
            gpu_mesh.vertex_count = uint32_t(mesh.vertices.size());
        }
        {
            const VkDeviceSize size = mesh.indices.size() * sizeof(mesh.indices[0]);
            VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            gpu_mesh.index_buffer = vk_create_buffer(size, usage, mesh.indices.data(), "index_buffer");
            gpu_mesh.index_count = uint32_t(mesh.indices.size());
        }
    }

    // Texture.
    {
        texture = vk_load_texture(get_resource_path("model/diffuse.jpg"));

        VkSamplerCreateInfo create_info { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        create_info.magFilter = VK_FILTER_LINEAR;
        create_info.minFilter = VK_FILTER_LINEAR;
        create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        create_info.mipLodBias = 0.0f;
        create_info.anisotropyEnable = VK_FALSE;
        create_info.maxAnisotropy = 1;
        create_info.minLod = 0.0f;
        create_info.maxLod = 12.0f;

        VK_CHECK(vkCreateSampler(vk.device, &create_info, nullptr, &sampler));
        vk_set_debug_name(sampler, "diffuse_texture_sampler");
    }

    uniform_buffer = vk_create_mapped_buffer(static_cast<VkDeviceSize>(sizeof(Matrix4x4)),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &mapped_uniform_buffer, "uniform_buffer");

    descriptor_set_layout = Vk_Descriptor_Set_Layout()
        .uniform_buffer(0, VK_SHADER_STAGE_VERTEX_BIT)
        .sampled_image(1, VK_SHADER_STAGE_FRAGMENT_BIT)
        .sampler(2, VK_SHADER_STAGE_FRAGMENT_BIT)
        .create("set_layout");

    pipeline_layout = vk_create_pipeline_layout({ descriptor_set_layout }, {}, "pipeline_layout");

    // Pipeline.
    Vk_Graphics_Pipeline_State state = get_default_graphics_pipeline_state();
    {
		Vk_Shader_Module vertex_shader(get_resource_path("spirv/mesh.vert.spv"));
		Vk_Shader_Module fragment_shader(get_resource_path("spirv/mesh.frag.spv"));

        // VkVertexInputBindingDescription
        state.vertex_bindings[0].binding = 0;
        state.vertex_bindings[0].stride = sizeof(Vertex);
        state.vertex_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        state.vertex_binding_count = 1;

        // VkVertexInputAttributeDescription
        state.vertex_attributes[0].location = 0; // position
        state.vertex_attributes[0].binding = 0;
        state.vertex_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        state.vertex_attributes[0].offset = 0;

        state.vertex_attributes[1].location = 1; // uv
        state.vertex_attributes[1].binding = 0;
        state.vertex_attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
        state.vertex_attributes[1].offset = 12;

        state.vertex_attribute_count = 2;

        state.color_attachment_formats[0] = vk.surface_format.format;
        state.color_attachment_count = 1;
        state.depth_attachment_format = get_depth_image_format();

        pipeline = vk_create_graphics_pipeline(state, vertex_shader.handle, fragment_shader.handle, pipeline_layout, "draw_mesh_pipeline");
    }

    // Descriptor buffer.
    {
        VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptor_buffer_properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT };
        VkPhysicalDeviceProperties2 physical_device_properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        physical_device_properties.pNext = &descriptor_buffer_properties;
        vkGetPhysicalDeviceProperties2(vk.physical_device, &physical_device_properties);

        VkDeviceSize layout_size_in_bytes = 0;
        vkGetDescriptorSetLayoutSizeEXT(vk.device, descriptor_set_layout, &layout_size_in_bytes);

        descriptor_buffer = vk_create_mapped_buffer(
            layout_size_in_bytes,
            VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT,
            &mapped_descriptor_buffer_ptr, "descriptor_buffer"
        );
        assert(descriptor_buffer.device_address % descriptor_buffer_properties.descriptorBufferOffsetAlignment == 0);

        // Write descriptor 0 (uniform buffer)
        {
            VkDescriptorAddressInfoEXT address_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
            address_info.address = uniform_buffer.device_address;
            address_info.range = sizeof(Matrix4x4);

            VkDescriptorGetInfoEXT descriptor_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
            descriptor_info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_info.data.pUniformBuffer = &address_info;

            VkDeviceSize offset;
            vkGetDescriptorSetLayoutBindingOffsetEXT(vk.device, descriptor_set_layout, 0, &offset);
            vkGetDescriptorEXT(vk.device, &descriptor_info, descriptor_buffer_properties.uniformBufferDescriptorSize,
                (uint8_t*)mapped_descriptor_buffer_ptr + offset);
        }

        // Write descriptor 1 (sampled image)
        {
            VkDescriptorImageInfo image_info;
            image_info.imageView = texture.view;
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkDescriptorGetInfoEXT descriptor_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
            descriptor_info.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptor_info.data.pSampledImage = &image_info;

            VkDeviceSize offset;
            vkGetDescriptorSetLayoutBindingOffsetEXT(vk.device, descriptor_set_layout, 1, &offset);
            vkGetDescriptorEXT(vk.device, &descriptor_info, descriptor_buffer_properties.sampledImageDescriptorSize,
                (uint8_t*)mapped_descriptor_buffer_ptr + offset);
        }

        // Write descriptor 2 (sampler)
        {
            VkDescriptorGetInfoEXT descriptor_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
            descriptor_info.type = VK_DESCRIPTOR_TYPE_SAMPLER;
            descriptor_info.data.pSampler = &sampler;

            VkDeviceSize offset;
            vkGetDescriptorSetLayoutBindingOffsetEXT(vk.device, descriptor_set_layout, 2, &offset);
            vkGetDescriptorEXT(vk.device, &descriptor_info, descriptor_buffer_properties.samplerDescriptorSize,
                (uint8_t*)mapped_descriptor_buffer_ptr + offset);
        }
    }

    // ImGui setup.
    {
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForVulkan(window, true);

        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance = vk.instance;
        init_info.PhysicalDevice = vk.physical_device;
        init_info.Device = vk.device;
        init_info.QueueFamily = vk.queue_family_index;
        init_info.Queue = vk.queue;
        init_info.DescriptorPool = vk.imgui_descriptor_pool;
		init_info.MinImageCount = 2;
		init_info.ImageCount = (uint32_t)vk.swapchain_info.images.size();
        init_info.UseDynamicRendering = true;
        init_info.PipelineRenderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        init_info.PipelineRenderingCreateInfo.colorAttachmentCount = state.color_attachment_count;
        init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = state.color_attachment_formats;
        init_info.PipelineRenderingCreateInfo.depthAttachmentFormat = state.depth_attachment_format;

        ImGui_ImplVulkan_Init(& init_info);
        ImGui::StyleColorsDark();
        ImGui_ImplVulkan_CreateFontsTexture();
    }

    restore_resolution_dependent_resources();
    gpu_times.frame = time_keeper.allocate_time_interval();
    time_keeper.initialize_time_intervals();
}

void Vk_Demo::shutdown() {
    VK_CHECK(vkDeviceWaitIdle(vk.device));

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    release_resolution_dependent_resources();
    gpu_mesh.destroy();
    texture.destroy();
    descriptor_buffer.destroy();
    uniform_buffer.destroy();

    vkDestroySampler(vk.device, sampler, nullptr);
    vkDestroyDescriptorSetLayout(vk.device, descriptor_set_layout, nullptr);
    vkDestroyPipelineLayout(vk.device, pipeline_layout, nullptr);
    vkDestroyPipeline(vk.device, pipeline, nullptr);

    vk_shutdown();
}

void Vk_Demo::release_resolution_dependent_resources() {
    depth_buffer_image.destroy();
}

void Vk_Demo::restore_resolution_dependent_resources() {
    // create depth buffer
    VkFormat depth_format = get_depth_image_format();
    depth_buffer_image = vk_create_image(vk.surface_size.width, vk.surface_size.height, depth_format,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, "depth_buffer");

    vk_execute(vk.command_pools[0], vk.queue, [this](VkCommandBuffer command_buffer) {
        const VkImageSubresourceRange subresource_range{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
        vk_cmd_image_barrier_for_subresource(command_buffer, depth_buffer_image.handle, subresource_range,
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    });

    last_frame_time = Clock::now();
}

void Vk_Demo::run_frame() {
    Time current_time = Clock::now();
    if (animate) {
        double time_delta = std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_frame_time).count() / 1e6;
        sim_time += time_delta;
    }
    last_frame_time = current_time;

	float aspect_ratio = (float)vk.surface_size.width / (float)vk.surface_size.height;
	Matrix4x4 projection_transform = perspective_transform_opengl_z01(radians(45.0f), aspect_ratio, 0.1f, 50.0f);
	Matrix3x4 view_transform = look_at_transform(camera_pos, Vector3(0), Vector3(0, 1, 0));
    Matrix3x4 model_transform = rotate_y(Matrix3x4::identity, (float)sim_time * radians(20.0f));
    Matrix4x4 model_view_proj = projection_transform * view_transform * model_transform;
    memcpy(mapped_uniform_buffer, &model_view_proj, sizeof(model_view_proj));

    do_imgui();
    draw_frame();
}

void Vk_Demo::draw_frame() {
    vk_begin_frame();
    vk_begin_gpu_marker_scope(vk.command_buffer, "draw_frame");
    time_keeper.next_frame();
    gpu_times.frame->begin();

    VkDescriptorBufferBindingInfoEXT descriptor_buffer_binding_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT };
    descriptor_buffer_binding_info.address = descriptor_buffer.device_address;
    descriptor_buffer_binding_info.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
    vkCmdBindDescriptorBuffersEXT(vk.command_buffer, 1, &descriptor_buffer_binding_info);

    vk_cmd_image_barrier(vk.command_buffer, vk.swapchain_info.images[vk.swapchain_image_index],
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT /* execution dependency with acquire semaphore wait */, VK_ACCESS_2_NONE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkViewport viewport{};
    viewport.width = float(vk.surface_size.width);
    viewport.height = float(vk.surface_size.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(vk.command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = vk.surface_size;
    vkCmdSetScissor(vk.command_buffer, 0, 1, &scissor);

    VkRenderingAttachmentInfo color_attachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    color_attachment.imageView = vk.swapchain_info.image_views[vk.swapchain_image_index];
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue.color = { 0.32f, 0.32f, 0.4f, 0.0f };

    VkRenderingAttachmentInfo depth_attachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    depth_attachment.imageView = depth_buffer_image.view;
    depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.clearValue.depthStencil = { 1.f, 0 };

    VkRenderingInfo rendering_info{ VK_STRUCTURE_TYPE_RENDERING_INFO };
    rendering_info.renderArea.extent = vk.surface_size;
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &color_attachment;
    rendering_info.pDepthAttachment = &depth_attachment;

    vkCmdBeginRendering(vk.command_buffer, &rendering_info);
    const VkDeviceSize zero_offset = 0;
    vkCmdBindVertexBuffers(vk.command_buffer, 0, 1, &gpu_mesh.vertex_buffer.handle, &zero_offset);
    vkCmdBindIndexBuffer(vk.command_buffer, gpu_mesh.index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

    const uint32_t buffer_index = 0;
    const VkDeviceSize set_offset = 0;
    vkCmdSetDescriptorBufferOffsetsEXT(vk.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &buffer_index, &set_offset);

    vkCmdBindPipeline(vk.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdDrawIndexed(vk.command_buffer, gpu_mesh.index_count, 1, 0, 0, 0);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vk.command_buffer);
    vkCmdEndRendering(vk.command_buffer);

    vk_cmd_image_barrier(vk.command_buffer, vk.swapchain_info.images[vk.swapchain_image_index],
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    gpu_times.frame->end();
    vk_end_gpu_marker_scope(vk.command_buffer);
    vk_end_frame();
}

void Vk_Demo::do_imgui() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if (ImGui::IsKeyPressed(ImGuiKey_F10)) {
            show_ui = !show_ui;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_W) || ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            camera_pos.z -= 0.2f;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_S) || ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            camera_pos.z += 0.2f;
        }
    }
    if (show_ui) {
        const float margin = 10.0f;
        static int corner = 0;

        ImVec2 window_pos = ImVec2((corner & 1) ? ImGui::GetIO().DisplaySize.x - margin : margin,
                                   (corner & 2) ? ImGui::GetIO().DisplaySize.y - margin : margin);
        ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);

        if (corner != -1) {
            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        }
        ImGui::SetNextWindowBgAlpha(0.3f);

        if (ImGui::Begin("UI", &show_ui, 
            (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
        {
            ImGui::Text("%.1f FPS (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
            ImGui::Text("Frame time: %.2f ms", gpu_times.frame->length_ms);
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Checkbox("Vertical sync", &vsync);
            ImGui::Checkbox("Animate", &animate);

            if (ImGui::BeginPopupContextWindow()) {
                if (ImGui::MenuItem("Custom",       NULL, corner == -1)) corner = -1;
                if (ImGui::MenuItem("Top-left",     NULL, corner == 0)) corner = 0;
                if (ImGui::MenuItem("Top-right",    NULL, corner == 1)) corner = 1;
                if (ImGui::MenuItem("Bottom-left",  NULL, corner == 2)) corner = 2;
                if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
                if (ImGui::MenuItem("Close")) show_ui = false;
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }
    ImGui::Render();
}
