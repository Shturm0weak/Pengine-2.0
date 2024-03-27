#include "VulkanBuffer.h"

#include "VulkanDevice.h"

#include "../Core/Logger.h"

using namespace Pengine;
using namespace Vk;

VkDeviceSize VulkanBuffer::GetAlignment(const VkDeviceSize instanceSize, const VkDeviceSize minOffsetAlignment)
{
	if (minOffsetAlignment > 0)
	{
		return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
	}

	return instanceSize;
}

std::shared_ptr<VulkanBuffer> VulkanBuffer::Create(const size_t instanceSize, const uint32_t instanceCount,
												   const Usage usage, const MemoryType memoryType)
{
	VkBufferUsageFlags bufferUsageFlags = ConvertUsage(usage) | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	VmaMemoryUsage memoryUsage{};
	VmaAllocationCreateFlags memoryFlags{};

	if (memoryType == MemoryType::CPU)
	{
		memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		memoryFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	}
	else if (memoryType == MemoryType::GPU)
	{
		memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	}

	return std::make_shared<VulkanBuffer>(
		instanceSize,
		instanceCount,
		bufferUsageFlags,
		memoryUsage,
		memoryFlags,
		memoryType);
}

std::shared_ptr<VulkanBuffer> VulkanBuffer::CreateStagingBuffer(
	const VkDeviceSize instanceSize,
	const uint32_t instanceCount)
{
	return std::make_shared<VulkanBuffer>(
		instanceSize,
		instanceCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
		VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		MemoryType::CPU);
}

VkBufferUsageFlagBits VulkanBuffer::ConvertUsage(const Usage usage)
{
    switch (usage)
    {
    case Pengine::Buffer::Usage::UNIFORM_BUFFER:
        return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    case Pengine::Buffer::Usage::VERTEX_BUFFER:
        return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    case Pengine::Buffer::Usage::INDEX_BUFFER:
        return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }

    FATAL_ERROR("Failed to convert buffer usage!");
}

Buffer::Usage VulkanBuffer::ConvertUsage(const VkBufferUsageFlagBits usage)
{
    switch (usage)
    {
    case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT:
        return Pengine::Buffer::Usage::UNIFORM_BUFFER;
    case VK_BUFFER_USAGE_VERTEX_BUFFER_BIT:
        return Pengine::Buffer::Usage::VERTEX_BUFFER;
    case VK_BUFFER_USAGE_INDEX_BUFFER_BIT:
        return Pengine::Buffer::Usage::INDEX_BUFFER;
    }

    FATAL_ERROR("Failed to convert buffer usage!");
	return {};
}

VulkanBuffer::VulkanBuffer(
    const VkDeviceSize instanceSize,
    const uint32_t instanceCount,
    const VkBufferUsageFlags bufferUsageFlags,
    const VmaMemoryUsage memoryUsage,
    const VmaAllocationCreateFlags memoryFlags,
    const MemoryType memoryType,
    const VkDeviceSize minOffsetAlignment)
	: Buffer(memoryType)
	, m_InstanceCount(instanceCount)
    , m_InstanceSize(instanceSize)
    , m_UsageFlags(bufferUsageFlags)
    , m_MemoryUsage(memoryUsage)
	, m_MemoryFlags(memoryFlags)
{
    m_AlignmentSize = GetAlignment(instanceSize, minOffsetAlignment);
	m_BufferSize = m_AlignmentSize * instanceCount;

    device->CreateBuffer(
    	m_BufferSize,
    	bufferUsageFlags,
    	memoryUsage,
    	memoryFlags,
    	m_Buffer,
    	m_VmaAllocation,
    	m_VmaAllocationInfo);
}

VulkanBuffer::~VulkanBuffer()
{
	vkDeviceWaitIdle(device->GetDevice());

	vmaDestroyBuffer(device->GetVmaAllocator(), m_Buffer, m_VmaAllocation);
}

void* VulkanBuffer::GetData() const
{
	// TODO: Make possible get data from GPU.
	if (m_MemoryType != MemoryType::CPU)
	{
		FATAL_ERROR("Can't get data from the buffer that is allocated on GPU!");
	}

	return m_VmaAllocationInfo.pMappedData;
}

void VulkanBuffer::WriteToBuffer(void *data, const size_t size, const size_t offset)
{
	if (m_MemoryType == MemoryType::CPU)
	{
		vmaCopyMemoryToAllocation(device->GetVmaAllocator(), data, m_VmaAllocation, offset, size);
	}
	else if (m_MemoryType == MemoryType::GPU)
	{
		const std::shared_ptr<VulkanBuffer> stagingBuffer = CreateStagingBuffer(
			size,
			1);

		stagingBuffer->WriteToBuffer(data, size, offset);

		device->CopyBuffer(
			stagingBuffer->m_Buffer,
			m_Buffer,
			size,
			offset);
	}
}

void VulkanBuffer::Copy(
	const std::shared_ptr<Buffer>& buffer,
	const size_t dstOffset)
{
    const std::shared_ptr<VulkanBuffer> vkBuffer = std::static_pointer_cast<VulkanBuffer>(buffer);

	if (GetSize() < vkBuffer->GetSize())
	{
		Logger::Error("Can't copy buffer, src size is buffer than dst size!");
	}

    device->CopyBuffer(
    	vkBuffer->GetBuffer(),
    	m_Buffer,
    	vkBuffer->GetSize(),
    	0,
    	dstOffset);
}

VkDescriptorBufferInfo VulkanBuffer::DescriptorInfo(const VkDeviceSize size, const VkDeviceSize offset) const
{
    return VkDescriptorBufferInfo{
        m_Buffer,
        offset,
        size,
    };
}