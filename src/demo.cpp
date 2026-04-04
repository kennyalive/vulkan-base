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
        VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME,
        VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME,
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

    VkPhysicalDeviceDescriptorHeapFeaturesEXT descriptor_heap_features{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT };
    descriptor_heap_features.descriptorHeap = VK_TRUE;
    pnexer.next(descriptor_heap_features);

    VkPhysicalDeviceShaderUntypedPointersFeaturesKHR shader_untyped_pointers_features{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR };
    shader_untyped_pointers_features.shaderUntypedPointers = VK_TRUE;
    pnexer.next(shader_untyped_pointers_features);

    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES };
    descriptor_indexing_features.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
    pnexer.next(descriptor_indexing_features);

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

    // Sampler information
    VkSamplerCreateInfo sampler_ci{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sampler_ci.magFilter = VK_FILTER_LINEAR;
    sampler_ci.minFilter = VK_FILTER_LINEAR;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_ci.mipLodBias = 0.0f;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.maxAnisotropy = 1;
    sampler_ci.minLod = 0.0f;
    sampler_ci.maxLod = 12.0f;

    // Texture.
    {
        texture = vk_load_texture(get_resource_path("model/diffuse.jpg"));
        VK_CHECK(vkCreateSampler(vk.device, &sampler_ci, nullptr, &sampler));
        vk_set_debug_name(sampler, "diffuse_texture_sampler");
    }

    const VkDeviceSize uniform_buffer_size = static_cast<VkDeviceSize>(sizeof(Matrix4x4));
    uniform_buffer = vk_create_mapped_buffer(uniform_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, "uniform_buffer");

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

        pipeline = vk_create_graphics_pipeline(state, vertex_shader.handle, fragment_shader.handle, "draw_mesh_pipeline");
    }

    // Descriptor heaps.
    {
        VkPhysicalDeviceDescriptorHeapPropertiesEXT descriptor_heap_properties{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT };
        VkPhysicalDeviceProperties2 physical_device_properties{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        physical_device_properties.pNext = &descriptor_heap_properties;

        vkGetPhysicalDeviceProperties2(vk.physical_device, &physical_device_properties);
        resource_reserved_size = descriptor_heap_properties.minResourceHeapReservedRange;
        sampler_reserved_size = descriptor_heap_properties.minSamplerHeapReservedRange;

        // Resource descriptor heap with two descriptors
        {
            const VkDeviceSize uniform_buffer_descriptor_offset = 0;
            
            VkDeviceSize image_descriptor_offset = round_up(
                uniform_buffer_descriptor_offset + descriptor_heap_properties.bufferDescriptorSize,
                descriptor_heap_properties.imageDescriptorAlignment
            );
            // TODO: until heap_offset is ready (allows to specify offset in bytes) we are going to
            // use "index offset". For that we need to ensure that image descriptor offset is not
            // only aligned to imageDescriptorAlignment but also aligned to image descriptor size
            // (so the offset of image descriptor array in bytes can be represented as multiple
            // of image descriptor size)
            image_descriptor_offset = round_up(image_descriptor_offset, descriptor_heap_properties.imageDescriptorSize);

            resource_reserved_offset = round_up(
                image_descriptor_offset + descriptor_heap_properties.imageDescriptorSize,
                std::max(descriptor_heap_properties.bufferDescriptorAlignment, descriptor_heap_properties.imageDescriptorAlignment)
            );
            resource_descriptor_heap_size = resource_reserved_offset + resource_reserved_size;
            std::vector<uint8_t> descriptor_data(resource_descriptor_heap_size);

            VkHostAddressRangeEXT descriptor_host_ranges[2] = {};
            descriptor_host_ranges[0].address = descriptor_data.data() + uniform_buffer_descriptor_offset;
            descriptor_host_ranges[0].size = descriptor_heap_properties.bufferDescriptorSize;
            descriptor_host_ranges[1].address = descriptor_data.data() + image_descriptor_offset;
            descriptor_host_ranges[1].size = descriptor_heap_properties.imageDescriptorSize;

            VkResourceDescriptorInfoEXT descriptor_infos[2] = {};

            // uniform buffer
            VkDeviceAddressRangeEXT uniform_buffer_range = { uniform_buffer.device_address, uniform_buffer_size };
            descriptor_infos[0].sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT;
            descriptor_infos[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_infos[0].data.pAddressRange = &uniform_buffer_range;

            // sampled image
            VkImageViewCreateInfo image_view_ci{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            image_view_ci.image = texture.handle;
            image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            image_view_ci.format = VK_FORMAT_R8G8B8A8_SRGB;
            image_view_ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, 1 };

            VkImageDescriptorInfoEXT image_descriptor_info{ VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT };
            image_descriptor_info.pView = &image_view_ci;
            image_descriptor_info.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            descriptor_infos[1].sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT;
            descriptor_infos[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptor_infos[1].data.pImage = &image_descriptor_info;

            vkWriteResourceDescriptorsEXT(vk.device, 2, descriptor_infos, descriptor_host_ranges);

            resource_descriptor_heap = vk_create_buffer_with_alignment(
                resource_descriptor_heap_size,
                VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
                (uint32_t)descriptor_heap_properties.resourceHeapAlignment,
                descriptor_data.data()
            );

            buffer_descriptor_index_offset = 0;
            image_descriptor_index_offset = uint32_t(image_descriptor_offset / descriptor_heap_properties.imageDescriptorSize);
        }

        // Sampler descriptor heap with a single sampler
        {
            const VkDeviceSize sampler_descriptor_offset = 0;
            sampler_reserved_offset = round_up(
                sampler_descriptor_offset + descriptor_heap_properties.samplerDescriptorSize,
                descriptor_heap_properties.samplerDescriptorAlignment
            );
            sampler_descriptor_heap_size = sampler_reserved_offset + sampler_reserved_size;
            std::vector<uint8_t> descriptor_data(sampler_descriptor_heap_size);

            VkHostAddressRangeEXT descriptor_host_range = {};
            descriptor_host_range.address = descriptor_data.data() + sampler_descriptor_offset;
            descriptor_host_range.size = descriptor_heap_properties.samplerDescriptorSize;
            vkWriteSamplerDescriptorsEXT(vk.device, 1, &sampler_ci, &descriptor_host_range);

            sampler_descriptor_heap = vk_create_buffer_with_alignment(
                sampler_descriptor_heap_size,
                VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
                (uint32_t)descriptor_heap_properties.samplerHeapAlignment,
                descriptor_data.data()
            );
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
    frame_timer = time_keeper.allocate_timer("frame");
    time_keeper.initialize_timers();
}

void Vk_Demo::shutdown() {
    VK_CHECK(vkDeviceWaitIdle(vk.device));

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    release_resolution_dependent_resources();
    gpu_mesh.destroy();
    texture.destroy();
    resource_descriptor_heap.destroy();
    sampler_descriptor_heap.destroy();
    uniform_buffer.destroy();

    vkDestroySampler(vk.device, sampler, nullptr);
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
    memcpy(uniform_buffer.mapped_ptr, &model_view_proj, sizeof(model_view_proj));

    do_imgui();
    draw_frame();
}

void Vk_Demo::draw_frame() {
    vk_begin_frame();
    vk_begin_marker(vk.command_buffer, "draw_frame");
    time_keeper.retrieve_query_results();
    frame_timer->start();

    VkBindHeapInfoEXT resource_heap_info{ VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT };
    resource_heap_info.heapRange.address = resource_descriptor_heap.device_address;
    resource_heap_info.heapRange.size = resource_descriptor_heap_size;
    resource_heap_info.reservedRangeOffset = resource_reserved_offset;
    resource_heap_info.reservedRangeSize = resource_reserved_size;
    vkCmdBindResourceHeapEXT(vk.command_buffer, &resource_heap_info);

    VkBindHeapInfoEXT sampler_heap_info{ VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT };
    sampler_heap_info.heapRange.address = sampler_descriptor_heap.device_address;
    sampler_heap_info.heapRange.size = sampler_descriptor_heap_size;
    sampler_heap_info.reservedRangeOffset = sampler_reserved_offset;
    sampler_heap_info.reservedRangeSize = sampler_reserved_size;
    vkCmdBindSamplerHeapEXT(vk.command_buffer, &sampler_heap_info);

    const uint32_t push_data[2] = { buffer_descriptor_index_offset , image_descriptor_index_offset };
    VkPushDataInfoEXT push_data_info{ VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT };
    push_data_info.data.address = push_data;
    push_data_info.data.size = 8;
    vkCmdPushDataEXT(vk.command_buffer, &push_data_info);

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
    vkCmdBindPipeline(vk.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdDrawIndexed(vk.command_buffer, gpu_mesh.index_count, 1, 0, 0, 0);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vk.command_buffer);
    vkCmdEndRendering(vk.command_buffer);

    vk_cmd_image_barrier(vk.command_buffer, vk.swapchain_info.images[vk.swapchain_image_index],
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    frame_timer->stop();
    vk_end_marker(vk.command_buffer);
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
            ImGui::Text("Frame time: %.2f ms", frame_timer->duration_ms);
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
