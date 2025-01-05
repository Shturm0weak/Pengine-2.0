#include "Buffer.h"

#include "../Vulkan/VulkanBuffer.h"
#include "../Core/Logger.h"

using namespace Pengine;

std::shared_ptr<Buffer> Buffer::Create(
	const size_t instanceSize,
	const uint32_t instanceCount,
	const Usage usage,
	const MemoryType memoryType,
	const bool isMultiBuffered)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return Vk::VulkanBuffer::Create(instanceSize, instanceCount, usage, memoryType, isMultiBuffered);
	}

	FATAL_ERROR("Failed to create the buffer, no graphics API implementation");
	return nullptr;
}

Buffer::Buffer(
	const MemoryType memoryType,
	const bool isMultiBuffered)
	: m_MemoryType(memoryType)
	, m_IsMultiBuffered(isMultiBuffered)
{
}