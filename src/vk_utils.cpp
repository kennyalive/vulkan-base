#include "vk_utils.h"
#include <cassert>

//
// Descriptor_Set_Layout
//
static VkDescriptorSetLayoutBinding get_set_layout_binding(uint32_t binding, VkDescriptorType descriptor_type, VkShaderStageFlags stage_flags) {
    VkDescriptorSetLayoutBinding entry{};
    entry.binding = binding;
    entry.descriptorType = descriptor_type;
    entry.descriptorCount = 1;
    entry.stageFlags = stage_flags;
    return entry;
}

Descriptor_Set_Layout& Descriptor_Set_Layout::sampled_image(uint32_t binding, VkShaderStageFlags stage_flags) {
    assert(binding_count < max_bindings);
    bindings[binding_count++] = get_set_layout_binding(binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, stage_flags);
    return *this;
}

Descriptor_Set_Layout& Descriptor_Set_Layout::storage_image(uint32_t binding, VkShaderStageFlags stage_flags) {
    assert(binding_count < max_bindings);
    bindings[binding_count++] = get_set_layout_binding(binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, stage_flags);
    return *this;
}

Descriptor_Set_Layout& Descriptor_Set_Layout::sampler(uint32_t binding, VkShaderStageFlags stage_flags) {
    assert(binding_count < max_bindings);
    bindings[binding_count++] = get_set_layout_binding(binding, VK_DESCRIPTOR_TYPE_SAMPLER, stage_flags);
    return *this;
}

Descriptor_Set_Layout& Descriptor_Set_Layout::uniform_buffer(uint32_t binding, VkShaderStageFlags stage_flags) {
    assert(binding_count < max_bindings);
    bindings[binding_count++] = get_set_layout_binding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage_flags);
    return *this;
}

Descriptor_Set_Layout& Descriptor_Set_Layout::storage_buffer(uint32_t binding, VkShaderStageFlags stage_flags) {
    assert(binding_count < max_bindings);
    bindings[binding_count++] = get_set_layout_binding(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stage_flags);
    return *this;
}

Descriptor_Set_Layout& Descriptor_Set_Layout::accelerator(uint32_t binding, VkShaderStageFlags stage_flags) {
    assert(binding_count < max_bindings);
    bindings[binding_count++] = get_set_layout_binding(binding, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, stage_flags);
    return *this;
}

VkDescriptorSetLayout Descriptor_Set_Layout::create(const char* name) {
    VkDescriptorSetLayoutCreateInfo create_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
    create_info.bindingCount = binding_count;
    create_info.pBindings = bindings;

    VkDescriptorSetLayout set_layout;
    VK_CHECK(vkCreateDescriptorSetLayout(vk.device, &create_info, nullptr, &set_layout));
    vk_set_debug_name(set_layout, name);
    return set_layout;
}

void GPU_Time_Interval::begin() {
    vkCmdWriteTimestamp(vk.command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vk.timestamp_query_pool, start_query[vk.frame_index]);
}
void GPU_Time_Interval::end() {
    vkCmdWriteTimestamp(vk.command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vk.timestamp_query_pool, start_query[vk.frame_index] + 1);
}

GPU_Time_Interval* GPU_Time_Keeper::allocate_time_interval() {
    assert(time_interval_count < max_time_intervals);
    GPU_Time_Interval* time_interval = &time_intervals[time_interval_count++];

    time_interval->start_query[0] = time_interval->start_query[1] = vk_allocate_timestamp_queries(2);
    time_interval->length_ms = 0.f;
    return time_interval;
}

void GPU_Time_Keeper::initialize_time_intervals() {
    vk_execute(vk.command_pools[0], vk.queue, [this](VkCommandBuffer command_buffer) {
        vkCmdResetQueryPool(command_buffer, vk.timestamp_query_pools[0], 0, 2 * time_interval_count);
        vkCmdResetQueryPool(command_buffer, vk.timestamp_query_pools[1], 0, 2 * time_interval_count);
        for (uint32_t i = 0; i < time_interval_count; i++) {
            vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vk.timestamp_query_pools[0], time_intervals[i].start_query[0]);
            vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vk.timestamp_query_pools[0], time_intervals[i].start_query[0] + 1);
            vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vk.timestamp_query_pools[1], time_intervals[i].start_query[1]);
            vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vk.timestamp_query_pools[1], time_intervals[i].start_query[1] + 1);
        }
    });
}

void GPU_Time_Keeper::next_frame() {
    uint64_t query_results[2/*query_result + availability*/ * 2/*start + end*/ * max_time_intervals];
    const uint32_t query_count = 2 * time_interval_count;
    VkResult result = vkGetQueryPoolResults(vk.device, vk.timestamp_query_pool, 0, query_count,
        query_count * 2*sizeof(uint64_t), query_results, 2*sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
    VK_CHECK_RESULT(result);
    assert(result != VK_NOT_READY);

    const float influence = 0.25f;

    for (uint32_t i = 0; i < time_interval_count; i++) {
        assert(query_results[4*i + 2] >= query_results[4*i]);
        time_intervals[i].length_ms = (1.f-influence) * time_intervals[i].length_ms + influence * float(double(query_results[4*i + 2] - query_results[4*i]) * vk.timestamp_period_ms);
    }

    vkCmdResetQueryPool(vk.command_buffer, vk.timestamp_query_pool, 0, query_count);
}

void begin_gpu_marker_scope(VkCommandBuffer command_buffer, const char* name) {
    VkDebugUtilsLabelEXT label { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
    label.pLabelName = name;
    vkCmdBeginDebugUtilsLabelEXT(command_buffer, &label);
}

void end_gpu_marker_scope(VkCommandBuffer command_buffer) {
    vkCmdEndDebugUtilsLabelEXT(command_buffer);
}

void write_gpu_marker(VkCommandBuffer command_buffer, const char* name) {
    VkDebugUtilsLabelEXT label { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
    label.pLabelName = name;
    vkCmdInsertDebugUtilsLabelEXT(command_buffer, &label);
}

