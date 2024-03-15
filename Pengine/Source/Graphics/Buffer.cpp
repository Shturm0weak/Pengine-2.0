#include "Buffer.h"

#include "../Vulkan/VulkanBuffer.h"

using namespace Pengine;

std::shared_ptr<Buffer> Buffer::Create(
	size_t instanceSize,
	uint32_t instanceCount,
	std::vector<Usage> usage)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return Vk::VulkanBuffer::Create(instanceSize, instanceCount, usage);
	}
}