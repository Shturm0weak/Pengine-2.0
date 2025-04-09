#include "Device.h"

#include "../Core/Logger.h"

#include "../Vulkan/VulkanDevice.h"

using namespace Pengine;

std::shared_ptr<Device> Device::Create(const std::string& applicationName)
{
	if (device)
	{
		Logger::Warning("Can't create a new device, the device is already created!");
		return device;
	}

	if (graphicsAPI == GraphicsAPI::Vk)
	{
		device = std::make_shared<Vk::VulkanDevice>(applicationName);
		return device;
	}

	FATAL_ERROR("Failed to create the device, no graphics API implementation");
	return nullptr;
}
