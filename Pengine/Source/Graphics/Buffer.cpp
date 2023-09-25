#include "Buffer.h"

#include "../SoftwareRenderer/SrBuffer.h"
#include "../Vulkan/VulkanBuffer.h"

using namespace Pengine;

std::shared_ptr<Buffer> Buffer::Create(uint64_t instanceSize,
	uint32_t instanceCount, std::vector<Usage> usage)
{
	/*if (graphicsAPI == GraphicsAPI::Software)
	{
		return std::make_shared<SrBuffer>(instanceSize * instanceCount);
	}
	else*/
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return Vk::VulkanBuffer::Create(instanceSize, instanceCount,
			usage);
	}
}