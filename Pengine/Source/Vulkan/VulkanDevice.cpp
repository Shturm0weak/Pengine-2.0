#include "VulkanDevice.h"

#include "../Core/Window.h"
#include "../Core/Logger.h"

#include "VulkanDescriptors.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <imgui/backends/imgui_impl_vulkan.h>

#include <cstring>
#include <iostream>
#include <set>
#include <unordered_set>

using namespace Pengine;
using namespace Vk;

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void *pUserData)
{
	Logger::Error(std::string("Validation layer: ") + pCallbackData->pMessage);
	
	return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
	const VkAllocationCallbacks *pAllocator,
	VkDebugUtilsMessengerEXT *pDebugMessenger)
{
	auto callback = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance,
		"vkCreateDebugUtilsMessengerEXT");

	if (callback != nullptr)
	{
		return callback(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks *pAllocator)
{
	auto callback = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance,
		"vkDestroyDebugUtilsMessengerEXT");
	
	if (callback != nullptr)
	{
		callback(instance, debugMessenger, pAllocator);
	}
}

void VulkanDevice::CreateInstance(const std::string& applicationName)
{
	if (enableValidationLayers && !CheckValidationLayerSupport())
	{
		FATAL_ERROR("Validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = applicationName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Pengine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = GetVulkanApiVersion();

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create instance!");
	}

	HasGflwRequiredInstanceExtensions();
}

void VulkanDevice::PickPhysicalDevice()
{
	uint32_t physicaldeviceCount = 0;
	vkEnumeratePhysicalDevices(m_Instance, &physicaldeviceCount, nullptr);

	if (physicaldeviceCount == 0)
	{
		FATAL_ERROR("Failed to find GPUs with Vulkan support!");
	}

	Logger::Log("Physical device count: " + std::to_string(physicaldeviceCount));

	std::vector<VkPhysicalDevice> devices(physicaldeviceCount);
	vkEnumeratePhysicalDevices(m_Instance, &physicaldeviceCount, devices.data());

	struct PhysicalDeviceInfo
	{
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties properties;
	};

	std::map<VkPhysicalDeviceType, PhysicalDeviceInfo> deviceInfoByType;

	Logger::Log("Physical devices:");

	for (const auto &device : devices)
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(device, &properties);

		if (IsDeviceSuitable(device))
		{
			Logger::Log(std::string(" * ") + properties.deviceName + " - Suitable");

			PhysicalDeviceInfo physicalDeviceInfo{};
			physicalDeviceInfo.device = device;
			physicalDeviceInfo.properties = properties;

			deviceInfoByType[properties.deviceType] = physicalDeviceInfo;
		}
		else
		{
			Logger::Log(std::string(" * ") + properties.deviceName + " - Not suitable");
		}
	}

	// Try to pick descrete gpu.
	{
		auto deviceInfo = deviceInfoByType.find(VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
		if (deviceInfo != deviceInfoByType.end())
		{
			m_PhysicalDevice = deviceInfo->second.device;
			m_PhysicalDeviceProperties = deviceInfo->second.properties;
		}
	}

	// Try to pick integrated gpu.
	if (m_PhysicalDevice == VK_NULL_HANDLE)
	{
		auto deviceInfo = deviceInfoByType.find(VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
		if (deviceInfo != deviceInfoByType.end())
		{
			m_PhysicalDevice = deviceInfo->second.device;
			m_PhysicalDeviceProperties = deviceInfo->second.properties;
		}
	}
	
	if (m_PhysicalDevice == VK_NULL_HANDLE)
	{
		FATAL_ERROR("Failed to find a suitable physical device!");
	}

	Logger::Log(std::string("Choosen physical device: ") + m_PhysicalDeviceProperties.deviceName, BOLDGREEN);
}

void VulkanDevice::CreateLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

	m_GraphicsFamilyIndex = indices.graphicsFamily;
	m_PresentFamilyIndex = indices.presentFamily;

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceFeatures.geometryShader = VK_TRUE;
	deviceFeatures.depthClamp = VK_TRUE;
	deviceFeatures.independentBlend = VK_TRUE;

	VkPhysicalDeviceVulkan12Features vulkan12Features{};
	vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

	vulkan12Features.scalarBlockLayout = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = &vulkan12Features;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create logical device!");
	}

	vkGetDeviceQueue(m_Device, indices.graphicsFamily, 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, indices.presentFamily, 0, &m_PresentQueue);
}

void VulkanDevice::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = FindPhysicalQueueFamilies();

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create command pool!");
	}
}

void VulkanDevice::CreateVmaAllocator()
{
	VmaAllocatorCreateInfo allocatorInfo = {};

	allocatorInfo.physicalDevice = m_PhysicalDevice;
	allocatorInfo.device = m_Device;
	allocatorInfo.instance = m_Instance;
	allocatorInfo.vulkanApiVersion = GetVulkanApiVersion();

	const VkResult result = vmaCreateAllocator(&allocatorInfo, &m_VmaAllocator);
	if (result != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create Vma allocator!");
	}
}

void VulkanDevice::CreateSurface(GLFWwindow* window)
{
	if (glfwCreateWindowSurface(m_Instance, window, nullptr, &m_Surface) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create window surface!");
	}
}

bool VulkanDevice::IsDeviceSuitable(const VkPhysicalDevice device)
{
	const QueueFamilyIndices indices = FindQueueFamilies(device);

	const bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() &&
			!swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
	
	return indices.IsComplete() && extensionsSupported && swapChainAdequate &&
			supportedFeatures.samplerAnisotropy;
}

void VulkanDevice::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr;  // Optional
}

void VulkanDevice::SetupDebugMessenger()
{
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) !=
		VK_SUCCESS)
	{
		FATAL_ERROR("Failed to set up debug messenger!");
	}
}

void VulkanDevice::SetupDebugUtilsLabel()
{
	m_VkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(
		m_Instance,
		"vkCmdBeginDebugUtilsLabelEXT");
	m_VkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(
		m_Instance,
		"vkCmdEndDebugUtilsLabelEXT");
}

bool VulkanDevice::CheckValidationLayerSupport() const
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const auto& layer : availableLayers)
	{
		std::cout << layer.layerName << std::endl;
	}

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto &layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

std::vector<const char*> VulkanDevice::GetRequiredExtensions() const
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

void VulkanDevice::HasGflwRequiredInstanceExtensions() const
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	Logger::Log("Available extensions:");
	std::unordered_set<std::string> available;

	for (const auto &extension : extensions)
	{
		Logger::Log(std::string("\t") + extension.extensionName);
		available.insert(extension.extensionName);
	}

	Logger::Log("Required extensions:");
	auto requiredExtensions = GetRequiredExtensions();

	for (const auto &required : requiredExtensions)
	{
		Logger::Log(std::string("\t") + required);
		if (available.find(required) == available.end())
		{
			FATAL_ERROR("Missing required glfw extension");
		}
	}
}

bool VulkanDevice::CheckDeviceExtensionSupport(const VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(
		device,
		nullptr,
		&extensionCount,
		availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

QueueFamilyIndices VulkanDevice::FindQueueFamilies(const VkPhysicalDevice device) const
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto &queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
			indices.graphicsFamilyHasValue = true;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport)
		{
			indices.presentFamily = i;
			indices.presentFamilyHasValue = true;
		}

		if (indices.IsComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

SwapChainSupportDetails VulkanDevice::QuerySwapChainSupport(const VkPhysicalDevice device) const
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount,
			details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device,
			m_Surface,
			&presentModeCount,
			details.presentModes.data());
	}

	return details;
}

VkFormat VulkanDevice::FindSupportedFormat(const std::vector<VkFormat> &candidates,
	const VkImageTiling tiling, const VkFormatFeatureFlags features) const
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && 
			(props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		if (tiling == VK_IMAGE_TILING_OPTIMAL &&
			(props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	FATAL_ERROR("Failed to find supported format!");
	return VkFormat::VK_FORMAT_UNDEFINED;
}

VulkanDevice::VulkanDevice(GLFWwindow* window, const std::string& applicationName)
{
	CreateInstance(applicationName);
	SetupDebugMessenger();
	SetupDebugUtilsLabel();
	CreateSurface(window);
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateVmaAllocator();
	CreateCommandPool();
}

VulkanDevice::~VulkanDevice()
{
	WaitIdle();

	if (enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
	}

	// Deleted via VulkanWindow::~VulkanWindow() -> ImGui_ImplVulkanH_DestroyWindow();
	//vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

	vmaDestroyAllocator(m_VmaAllocator);
	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
	vkDestroyDevice(m_Device, nullptr);
	vkDestroyInstance(m_Instance, nullptr);
}

uint32_t VulkanDevice::FindMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
	
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	FATAL_ERROR("Failed to find suitable memory type!");
	return -1;
}

void VulkanDevice::CreateBuffer(
	const VkDeviceSize size,
	const VkBufferUsageFlags bufferUsage,
	const VmaMemoryUsage memoryUsage,
	const VmaAllocationCreateFlags memoryFlags,
	VkBuffer& buffer,
	VmaAllocation& vmaAllocation,
	VmaAllocationInfo& vmaAllocationInfo) const
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = bufferUsage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.usage = memoryUsage;
	allocationCreateInfo.flags = memoryFlags;

	if (vmaCreateBuffer(
		m_VmaAllocator,
		&bufferCreateInfo,
		&allocationCreateInfo,
		&buffer,
		&vmaAllocation,
		&vmaAllocationInfo) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create buffer!");
	}
}

VkCommandBuffer VulkanDevice::BeginSingleTimeCommands() const
{
	m_Mutex.lock();
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanDevice::EndSingleTimeCommands(const VkCommandBuffer commandBuffer) const
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_GraphicsQueue);

	vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
	m_Mutex.unlock();
}

void VulkanDevice::CopyBuffer(
	const VkBuffer srcBuffer,
	const VkBuffer dstBuffer,
	const VkDeviceSize size,
	const VkDeviceSize srcOffset,
	const VkDeviceSize dstOffset) const
{
	const VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = srcOffset;
	copyRegion.dstOffset = dstOffset;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

void VulkanDevice::CopyBufferToImage(
	const VkBuffer buffer,
	const VkImage image,
	const uint32_t width,
	const uint32_t height)
{
	const VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region);

	EndSingleTimeCommands(commandBuffer);
}

void VulkanDevice::CopyImageToImage(
	VkImage src,
	VkImageLayout srcImageLayout,
	VkImage dst,
	VkImageLayout dstImageLayout,
	uint32_t width,
	uint32_t height,
	VkCommandBuffer commandBuffer)
{
	bool isCreatedSingleTimeCommands = false;
	if (commandBuffer == VK_NULL_HANDLE)
	{
		commandBuffer = BeginSingleTimeCommands();
		isCreatedSingleTimeCommands = true;
	}

	VkImageCopy region{};

	region.dstOffset = { 0, 0, 0 };
	region.srcOffset = { 0, 0, 0 };
	region.extent = { width, height, 1 };

	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.baseArrayLayer = 0;
	region.srcSubresource.layerCount = 1;
	region.srcSubresource.mipLevel = 0;

	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.baseArrayLayer = 0;
	region.dstSubresource.layerCount = 1;
	region.dstSubresource.mipLevel = 0;

	vkCmdCopyImage(commandBuffer, src, srcImageLayout, dst, dstImageLayout, 1, &region);

	if (isCreatedSingleTimeCommands)
	{
		EndSingleTimeCommands(commandBuffer);
	}
}

void VulkanDevice::CreateImage(
	const VkImageCreateInfo& imageInfo,
	VkImage& image,
	VmaAllocation& vmaAllocation,
	VmaAllocationInfo& vmaAllocationInfo) const
{
	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	
	if (vmaCreateImage(
		m_VmaAllocator,
		&imageInfo,
		&allocationCreateInfo,
		&image,
		&vmaAllocation,
		&vmaAllocationInfo) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create image!");
	}
}

void VulkanDevice::TransitionImageLayout(
	const VkImage image,
	const VkFormat format,
	const VkImageAspectFlagBits aspectMask,
	const uint32_t mipLevels,
	uint32_t layerCount,
	const VkImageLayout oldLayout,
	const VkImageLayout newLayout,
	VkCommandBuffer commandBuffer) const
{
	bool isCreatedSingleTimeCommands = false;
	if (commandBuffer == VK_NULL_HANDLE)
	{
		commandBuffer = BeginSingleTimeCommands();
		isCreatedSingleTimeCommands = true;
	}

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspectMask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layerCount;

	switch (oldLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		barrier.srcAccessMask = 0;
		break;
	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		break;
	}

	switch (newLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		if (barrier.srcAccessMask == 0)
		{
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		break;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	if (isCreatedSingleTimeCommands)
	{
		EndSingleTimeCommands(commandBuffer);
	}
}

void VulkanDevice::GenerateMipMaps(
	const VkImage image,
	const VkFormat imageFormat,
	const int32_t texWidth,
	const int32_t texHeight,
	const uint32_t mipLevels,
	uint32_t layerCount,
	VkCommandBuffer commandBuffer) const
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		FATAL_ERROR("Texture image format does not support linear blitting!");
	}

	bool isCreatedSingleTimeCommands = false;
	if (commandBuffer == VK_NULL_HANDLE)
	{
		commandBuffer = BeginSingleTimeCommands();
		isCreatedSingleTimeCommands = true;
	}
	
	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t face = 0; face < layerCount; face++)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = face;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		for (uint32_t mipLevel = 1; mipLevel < mipLevels; mipLevel++)
		{
			barrier.subresourceRange.baseMipLevel = mipLevel - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = mipLevel - 1;
			blit.srcSubresource.baseArrayLayer = face;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = mipLevel;
			blit.dstSubresource.baseArrayLayer = face;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	}

	if (isCreatedSingleTimeCommands)
	{
		EndSingleTimeCommands(commandBuffer);
	}
}

void VulkanDevice::CommandBeginLabel(
	const std::string& name,
	const VkCommandBuffer commandBuffer,
	const glm::vec3& color) const
{
	if (!m_VkCmdBeginDebugUtilsLabelEXT)
	{
		return;
	}

	VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
	label.pLabelName = name.c_str();
	label.color[0] = color[0];
	label.color[1] = color[1];
	label.color[2] = color[2];
	label.color[3] = 1.0f;
	m_VkCmdBeginDebugUtilsLabelEXT(commandBuffer, &label);
}

void VulkanDevice::CommandEndLabel(const VkCommandBuffer commandBuffer) const
{
	if (m_VkCmdEndDebugUtilsLabelEXT)
	{
		m_VkCmdEndDebugUtilsLabelEXT(commandBuffer);
	}
}

void VulkanDevice::WaitIdle() const
{
	std::lock_guard lock(m_Mutex);
	vkDeviceWaitIdle(m_Device);
}

void VulkanDevice::DeleteResource(std::function<void()>&& callback)
{
	Lock lock;
	m_DeletionQueue[currentFrame].emplace_back(std::move(callback));
}

void VulkanDevice::FlushDeletionQueue(bool immediate)
{
	std::vector<size_t> queuesToDelete;
	for (auto& [frame, queue] : m_DeletionQueue)
	{
		if (!immediate && currentFrame <= frame + swapChainImageCount)
		{
			continue;
		}

		while (!queue.empty())
		{
			auto callback = std::move(queue.front());
			queue.pop_front();
			callback();
		}

		queuesToDelete.emplace_back(frame);
	}

	for (const auto& frame : queuesToDelete)
	{
		m_DeletionQueue.erase(frame);
	}
}

VkCommandBuffer VulkanDevice::GetCommandBufferFromFrame(void* frame)
{
	if (frame)
	{
		const ImGui_ImplVulkanH_Frame* vkFrame = static_cast<ImGui_ImplVulkanH_Frame*>(frame);
		return vkFrame->CommandBuffer;
	}

	return VK_NULL_HANDLE;
}
