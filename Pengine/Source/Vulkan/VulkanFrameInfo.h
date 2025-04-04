#pragma once

#include <vulkan/vulkan.h>

struct VulkanFrameInfo
{
    VkCommandBuffer     CommandBuffer;
    VkCommandPool       CommandPool;
    VkFence             Fence;
};
