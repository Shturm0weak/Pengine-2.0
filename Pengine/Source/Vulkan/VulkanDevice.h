#pragma once

#include "../Core/Core.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>

#include <vma/vk_mem_alloc.h>

#include <vector>

namespace Pengine
{

    namespace Vk
    {

        struct PENGINE_API SwapChainSupportDetails
        {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        struct PENGINE_API QueueFamilyIndices
        {
            uint32_t graphicsFamily;
            uint32_t presentFamily;
            bool graphicsFamilyHasValue = false;
            bool presentFamilyHasValue = false;

            bool IsComplete() const { return graphicsFamilyHasValue && presentFamilyHasValue; }
        };

        class PENGINE_API VulkanDevice
        {
        public:
#ifdef NDEBUG
            const bool enableValidationLayers = false;
#else
            const bool enableValidationLayers = true;
#endif
            VulkanDevice(GLFWwindow* window, const std::string& applicationName);
            ~VulkanDevice();
            VulkanDevice(const VulkanDevice&) = delete;
            VulkanDevice& operator=(const VulkanDevice&) = delete;

            VkCommandPool GetCommandPool() const { return m_CommandPool; }

            VkDevice GetDevice() const { return m_Device; }

            VkSurfaceKHR GetSurface() const { return m_Surface; }

            VkInstance GetInstance() const { return m_Instance; }

            VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }

            VkQueue GetPresentQueue() const { return m_PresentQueue; }

            uint32_t GetGraphicsFamilyIndex() const { return m_GraphicsFamilyIndex; }

            uint32_t GetPresentFamilyIndex() const { return m_PresentFamilyIndex; }

            VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

            SwapChainSupportDetails GetSwapChainSupport() { return QuerySwapChainSupport(m_PhysicalDevice); }

            uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

            QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(m_PhysicalDevice); }

            VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates,
                VkImageTiling tiling, VkFormatFeatureFlags features);

            void CreateBuffer(
                VkDeviceSize size,
                VkBufferUsageFlags usage,
                VkMemoryPropertyFlags properties,
                VkBuffer& buffer,
                VkDeviceMemory& bufferMemory);

            VkCommandBuffer BeginSingleTimeCommands();

            void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

            void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

            void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,
                uint32_t layerCount);

            void CreateImageWithInfo(
                const VkImageCreateInfo& imageInfo,
                VkMemoryPropertyFlags properties,
                VkImage& image,
                VkDeviceMemory& imageMemory);

            void TransitionImageLayout(
                VkImage image,
                VkFormat format,
                VkImageAspectFlagBits aspectMask,
                VkImageLayout oldLayout,
                VkImageLayout newLayout,
                uint32_t mipLevels = 1);

            void GenerateMipMaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

            VkPhysicalDeviceProperties properties;

            void CommandBeginLabel(const std::string& name, VkCommandBuffer commandBuffer,
                const glm::vec4& color);

            void CommandEndLabel(VkCommandBuffer commandBuffer);

        private:
            void CreateInstance(const std::string& applicationName);

            void SetupDebugMessenger();

            void SetupDebugUtilsLabel();

            void CreateSurface(GLFWwindow* window);

            void PickPhysicalDevice();

            void CreateLogicalDevice();

            void CreateCommandPool();

            void CreateVmaAllocator();

            bool IsDeviceSuitable(VkPhysicalDevice device);

            std::vector<const char*> GetRequiredExtensions();

            bool CheckValidationLayerSupport();

            QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

            void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

            void HasGflwRequiredInstanceExtensions();

            bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

            SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

            VkInstance m_Instance;
            VkDebugUtilsMessengerEXT m_DebugMessenger;
            VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
            VkCommandPool m_CommandPool;
            VmaAllocator m_Allocator;

            VkDevice m_Device;
            VkSurfaceKHR m_Surface;
            VkQueue m_GraphicsQueue;
            VkQueue m_PresentQueue;
            uint32_t m_GraphicsFamilyIndex;
            uint32_t m_PresentFamilyIndex;

            PFN_vkCmdBeginDebugUtilsLabelEXT m_VkCmdBeginDebugUtilsLabelEXT;
            PFN_vkCmdEndDebugUtilsLabelEXT m_VkCmdEndDebugUtilsLabelEXT;

            const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
            const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        };

    }

}