#pragma once

#include "../Core/Core.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vma/vk_mem_alloc.h>

#include <deque>
#include <mutex>
#include <vector>

namespace Pengine::Vk
{

	struct PENGINE_API SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct PENGINE_API QueueFamilyIndices
	{
		uint32_t graphicsFamily = 0;
		uint32_t presentFamily = 0;
		bool graphicsFamilyHasValue = false;
		bool presentFamilyHasValue = false;

		[[nodiscard]] bool IsComplete() const { return graphicsFamilyHasValue && presentFamilyHasValue; }
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

		[[nodiscard]] VkCommandPool GetCommandPool() const { return m_CommandPool; }

		[[nodiscard]] VkDevice GetDevice() const { return m_Device; }

		[[nodiscard]] VkSurfaceKHR GetSurface() const { return m_Surface; }

		[[nodiscard]] VkInstance GetInstance() const { return m_Instance; }

		[[nodiscard]] VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }

		[[nodiscard]] VkQueue GetPresentQueue() const { return m_PresentQueue; }

		[[nodiscard]] uint32_t GetGraphicsFamilyIndex() const { return m_GraphicsFamilyIndex; }

		[[nodiscard]] uint32_t GetPresentFamilyIndex() const { return m_PresentFamilyIndex; }

		[[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

		[[nodiscard]] SwapChainSupportDetails GetSwapChainSupport() const { return QuerySwapChainSupport(m_PhysicalDevice); }

		[[nodiscard]] uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

		[[nodiscard]] QueueFamilyIndices FindPhysicalQueueFamilies() const { return FindQueueFamilies(m_PhysicalDevice); }

		[[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates,
			VkImageTiling tiling, VkFormatFeatureFlags features) const;

		[[nodiscard]] uint32_t GetVulkanApiVersion() const { return m_ApiVersion; }

		[[nodiscard]] VmaAllocator GetVmaAllocator() const { return m_VmaAllocator; }

		void CreateBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags bufferUsage,
			VmaMemoryUsage memoryUsage,
			VmaAllocationCreateFlags memoryFlags,
			VkBuffer& buffer,
			VmaAllocation& vmaAllocation,
			VmaAllocationInfo& vmaAllocationInfo) const;

		[[nodiscard]] VkCommandBuffer BeginSingleTimeCommands() const;

		void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;

		void CopyBuffer(
			VkBuffer srcBuffer,
			VkBuffer dstBuffer,
			VkDeviceSize size,
			VkDeviceSize srcOffset = 0,
			VkDeviceSize dstOffset = 0) const;

		void CopyBufferToImage(
			VkBuffer buffer,
			VkImage image,
			uint32_t width,
			uint32_t height);

		void CopyImageToImage(
			VkImage src,
			VkImageLayout srcImageLayout,
			VkImage dst,
			VkImageLayout dstImageLayout,
			uint32_t width,
			uint32_t height,
			VkCommandBuffer commandBuffer);

		void CreateImage(
			const VkImageCreateInfo& imageInfo,
			VkImage& image,
			VmaAllocation& vmaAllocation,
			VmaAllocationInfo& vmaAllocationInfo) const;

		void TransitionImageLayout(
			VkImage image,
			VkFormat format,
			VkImageAspectFlagBits aspectMask,
			uint32_t mipLevels,
			uint32_t layerCount,
			VkImageLayout oldLayout,
			VkImageLayout newLayout,
			VkCommandBuffer commandBuffer) const;

		void GenerateMipMaps(
			VkImage image,
			VkFormat imageFormat,
			int32_t texWidth,
			int32_t texHeight,
			uint32_t mipLevels,
			uint32_t layerCount,
			VkCommandBuffer commandBuffer) const;

		void CommandBeginLabel(
			const std::string& name,
			VkCommandBuffer commandBuffer,
			const glm::vec3& color) const;

		void CommandEndLabel(VkCommandBuffer commandBuffer) const;

		void WaitIdle() const;

		void DeleteResource(std::function<void()>&& callback);

		void FlushDeletionQueue(bool immediate = false);

		VkCommandBuffer GetCommandBufferFromFrame(void* frame);
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

		[[nodiscard]] std::vector<const char*> GetRequiredExtensions() const;

		[[nodiscard]] bool CheckValidationLayerSupport() const;

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

		static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		void HasGflwRequiredInstanceExtensions() const;

		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

		[[nodiscard]] SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;

		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
		VkCommandPool m_CommandPool;

		VkDevice m_Device;
		VkSurfaceKHR m_Surface;
		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;
		uint32_t m_GraphicsFamilyIndex;
		uint32_t m_PresentFamilyIndex;

		uint32_t m_ApiVersion = VK_API_VERSION_1_3;

		VmaAllocator m_VmaAllocator;

		PFN_vkCmdBeginDebugUtilsLabelEXT m_VkCmdBeginDebugUtilsLabelEXT;
		PFN_vkCmdEndDebugUtilsLabelEXT m_VkCmdEndDebugUtilsLabelEXT;

		const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
		const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		mutable std::mutex m_Mutex;

		std::unordered_map<size_t, std::deque<std::function<void()>>> m_DeletionQueue;

	public:
		class Lock
		{
		public:
			Lock()
			{
				device->m_Mutex.lock();
			}

			~Lock()
			{
				device->m_Mutex.unlock();
			}
		};
	};

}