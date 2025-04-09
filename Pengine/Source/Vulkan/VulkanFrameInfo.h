#pragma once

#include <vulkan/vulkan.h>

struct VulkanFrameInfo
{
    VkCommandBuffer     CommandBuffer = VK_NULL_HANDLE;
    VkCommandPool       CommandPool   = VK_NULL_HANDLE;
    VkFence             Fence         = VK_NULL_HANDLE;
};
