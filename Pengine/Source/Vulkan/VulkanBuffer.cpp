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

void VulkanBuffer::WriteToVulkanBuffer(
	const uint32_t imageIndex,
	void* data,
	const size_t size,
	const size_t offset)
{
	if (m_MemoryType == MemoryType::CPU)
	{
		vmaCopyMemoryToAllocation(device->GetVmaAllocator(), data, m_BufferDatas[imageIndex].m_VmaAllocation, offset, size);
	}
	else if (m_MemoryType == MemoryType::GPU)
	{
		const std::shared_ptr<VulkanBuffer> stagingBuffer = CreateStagingBuffer(
			size,
			1);

		stagingBuffer->WriteToBuffer(data, size, offset);

		device->CopyBuffer(
			stagingBuffer->m_BufferDatas[imageIndex].m_Buffer,
			m_BufferDatas[imageIndex].m_Buffer,
			size,
			offset);
	}
}

std::shared_ptr<VulkanBuffer> VulkanBuffer::Create(
	const size_t instanceSize,
	const uint32_t instanceCount,
	const Usage usage,
	const MemoryType memoryType,
	const bool isMultiBuffered)
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
		memoryType,
		usage == Usage::UNIFORM_BUFFER ? true : isMultiBuffered);
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
		MemoryType::CPU,
		false);
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
	const bool isMultiBuffered,
	const VkDeviceSize minOffsetAlignment)
	: Buffer(memoryType, isMultiBuffered)
	, m_InstanceCount(instanceCount)
	, m_InstanceSize(instanceSize)
	, m_UsageFlags(bufferUsageFlags)
	, m_MemoryUsage(memoryUsage)
	, m_MemoryFlags(memoryFlags)
{
	m_AlignmentSize = GetAlignment(instanceSize, minOffsetAlignment);
	m_BufferSize = m_AlignmentSize * instanceCount;

	if (m_IsMultiBuffered)
	{
		m_Data = new uint8_t[m_BufferSize];
	}

	m_IsChanged.resize(m_IsMultiBuffered ? swapChainImageCount : 1, false);
	m_BufferDatas.resize(m_IsMultiBuffered ? swapChainImageCount : 1);
	for (BufferData& bufferData : m_BufferDatas)
	{
		device->CreateBuffer(
			m_BufferSize,
			bufferUsageFlags,
			memoryUsage,
			memoryFlags,
			bufferData.m_Buffer,
			bufferData.m_VmaAllocation,
			bufferData.m_VmaAllocationInfo);
	}
}

VulkanBuffer::~VulkanBuffer()
{
	for (BufferData bufferData : m_BufferDatas)
	{
		device->DeleteResource([bufferData]()
		{
			vmaDestroyBuffer(device->GetVmaAllocator(), bufferData.m_Buffer, bufferData.m_VmaAllocation);
		});
	}

	if (m_Data)
	{
		delete[] m_Data;
		m_Data = nullptr;
	}
}

void* VulkanBuffer::GetData() const
{
	if (m_IsMultiBuffered)
	{
		return m_Data;
	}

	// TODO: Make possible get data from GPU.
	if (m_MemoryType != MemoryType::CPU)
	{
		FATAL_ERROR("Can't get data from the buffer that is allocated on GPU!");
	}

	return m_BufferDatas[swapChainImageIndex].m_VmaAllocationInfo.pMappedData;
}

void VulkanBuffer::WriteToBuffer(void* data, const size_t size, const size_t offset)
{
	if (m_IsMultiBuffered)
	{
		m_IsChanged.assign(m_IsChanged.size(), true);
		memcpy((void*)&m_Data[offset], data, size);
	}
	else
	{
		WriteToVulkanBuffer(0, data, size, offset);
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

	m_IsChanged.assign(m_IsChanged.size(), true);

	if (m_IsMultiBuffered)
	{
		memcpy((void*)m_Data[dstOffset], vkBuffer->m_Data, GetSize());
	}
	else
	{
		device->CopyBuffer(
			vkBuffer->GetBuffer(),
			m_BufferDatas.back().m_Buffer,
			vkBuffer->GetSize(),
			0,
			dstOffset);
	}
}

void VulkanBuffer::Flush()
{
	if (!m_IsMultiBuffered)
	{
		return;
	}

	const uint32_t imageIndex = m_IsChanged.size() == 1 ? 0 : swapChainImageIndex;

	if (!m_IsChanged[imageIndex])
	{
		return;
	}

	// TODO: can be done more optimal by coping just updated part of the buffer.
	if (m_MemoryType == MemoryType::CPU)
	{
		vmaCopyMemoryToAllocation(
			device->GetVmaAllocator(),
			m_Data,
			m_BufferDatas[imageIndex].m_VmaAllocation,
			0,
			GetSize());
	}
	else if (m_MemoryType == MemoryType::GPU)
	{
		const std::shared_ptr<VulkanBuffer> stagingBuffer = CreateStagingBuffer(
			GetSize(),
			1);

		stagingBuffer->WriteToBuffer(m_Data, GetSize(), 0);

		device->CopyBuffer(
			stagingBuffer->m_BufferDatas.begin()->m_Buffer,
			m_BufferDatas[imageIndex].m_Buffer,
			GetSize(),
			0);
	}

	m_IsChanged[imageIndex] = false;
}

VkDescriptorBufferInfo VulkanBuffer::DescriptorInfo(
	const uint32_t imageIndex,
	const VkDeviceSize size,
	const VkDeviceSize offset) const
{
	return VkDescriptorBufferInfo{
		m_BufferDatas[imageIndex].m_Buffer,
		offset,
		size,
	};
}

VkBuffer VulkanBuffer::GetBuffer() const
{
	if (m_IsMultiBuffered)
	{
		return m_BufferDatas[swapChainImageIndex].m_Buffer;
	}
	else
	{
		return m_BufferDatas.back().m_Buffer;
	}
}
