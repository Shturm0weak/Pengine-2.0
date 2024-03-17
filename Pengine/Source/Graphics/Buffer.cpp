#include "Buffer.h"

#include "../Vulkan/VulkanBuffer.h"
#include "../Core/Logger.h"

using namespace Pengine;

std::shared_ptr<Buffer> Buffer::Create(
	const size_t instanceSize,
	const uint32_t instanceCount,
	const std::vector<Usage>& usage)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return Vk::VulkanBuffer::Create(instanceSize, instanceCount, usage);
	}

	FATAL_ERROR("Failed to create the buffer, no graphics API implementation");
	return nullptr;
}