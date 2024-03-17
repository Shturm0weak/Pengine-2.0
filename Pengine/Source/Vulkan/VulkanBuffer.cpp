#include "VulkanBuffer.h"

#include "VulkanDevice.h"

#include "../Core/Logger.h"

using namespace Pengine;
using namespace Vk;

/**
 * Returns the minimum instance size required to be compatible with devices minOffsetAlignment
 *
 * @param instanceSize The size of an instance
 * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
 * minUniformBufferOffsetAlignment)
 *
 * @return VkResult of the buffer mapping call
 */
VkDeviceSize VulkanBuffer::GetAlignment(
    const VkDeviceSize instanceSize,
    const VkDeviceSize minOffsetAlignment)
{
    if (minOffsetAlignment > 0)
    {
        return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
    }

    return instanceSize;
}

std::shared_ptr<VulkanBuffer> VulkanBuffer::Create(
    const size_t instanceSize,
    const uint32_t instanceCount,
    const std::vector<Usage>& usage)
{
    bool isTransferSrc = false;

    VkBufferUsageFlags usageFlags{};
    for (const Usage usageFlag : usage)
    {
        usageFlags |= ConvertUsage(usageFlag);

        if (usageFlag == Usage::TRANSFER_SRC)
        {
            isTransferSrc = true;
        }
    }

    VkMemoryPropertyFlags memoryFlags;
    if (isTransferSrc)
    {
        memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    else
    {
        memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    return std::make_shared<VulkanBuffer>(instanceSize, instanceCount, usageFlags, memoryFlags);
}

VkBufferUsageFlagBits VulkanBuffer::ConvertUsage(const Usage usage)
{
    switch (usage)
    {
    case Pengine::Buffer::Usage::STORAGE_BUFFER:
        return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    case Pengine::Buffer::Usage::UNIFORM_BUFFER:
        return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    case Pengine::Buffer::Usage::VERTEX_BUFFER:
        return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    case Pengine::Buffer::Usage::INDEX_BUFFER:
        return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    case Pengine::Buffer::Usage::TRANSFER_SRC:
        return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    case Pengine::Buffer::Usage::TRANSFER_DST:
        return VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    FATAL_ERROR("Failed to convert buffer usage!");
}

Buffer::Usage VulkanBuffer::ConvertUsage(const VkBufferUsageFlagBits usage)
{
    switch (usage)
    {
    case VK_BUFFER_USAGE_STORAGE_BUFFER_BIT:
        return Pengine::Buffer::Usage::STORAGE_BUFFER;
    case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT:
        return Pengine::Buffer::Usage::UNIFORM_BUFFER;
    case VK_BUFFER_USAGE_VERTEX_BUFFER_BIT:
        return Pengine::Buffer::Usage::VERTEX_BUFFER;
    case VK_BUFFER_USAGE_INDEX_BUFFER_BIT:
        return Pengine::Buffer::Usage::INDEX_BUFFER;
    case VK_BUFFER_USAGE_TRANSFER_SRC_BIT:
        return Pengine::Buffer::Usage::TRANSFER_SRC;
    case VK_BUFFER_USAGE_TRANSFER_DST_BIT:
        return Pengine::Buffer::Usage::TRANSFER_DST;
    }

    FATAL_ERROR("Failed to convert buffer usage!");
	return {};
}

VulkanBuffer::VulkanBuffer(
    const VkDeviceSize instanceSize,
    const uint32_t instanceCount,
    const VkBufferUsageFlags usageFlags,
    const VkMemoryPropertyFlags memoryPropertyFlags,
    const VkDeviceSize minOffsetAlignment)
	: m_InstanceCount(instanceCount)
    , m_InstanceSize(instanceSize)
    , m_UsageFlags(usageFlags)
    , m_MemoryPropertyFlags(memoryPropertyFlags)
{
    m_AlignmentSize = GetAlignment(instanceSize, minOffsetAlignment);
	m_BufferSize = m_AlignmentSize * instanceCount;
    device->CreateBuffer(m_BufferSize, usageFlags, memoryPropertyFlags, m_Buffer, m_Memory);
}

VulkanBuffer::~VulkanBuffer()
{
    vkDeviceWaitIdle(device->GetDevice());

    Unmap();
    vkDestroyBuffer(device->GetDevice(), m_Buffer, nullptr);
    vkFreeMemory(device->GetDevice(), m_Memory, nullptr);
}

void* VulkanBuffer::GetData()
{
    return m_Mapped;
}

/**
 * Copies the specified data to the mapped buffer. Default value writes whole buffer range
 *
 * @param data Pointer to the data to copy
 * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
 * range.
 * @param offset (Optional) Byte offset from beginning of mapped region
 *
 */
void VulkanBuffer::WriteToBuffer(void *data, const size_t size, const size_t offset)
{
    Map(size, offset);

    assert(m_Mapped && "Cannot copy to unmapped buffer");

    if (size == VK_WHOLE_SIZE)
    {
        memcpy(m_Mapped, data, m_BufferSize);
    }
    else
    {
        char* memOffset = static_cast<char*>(m_Mapped);
        memOffset += offset;
        memcpy(memOffset, data, size);
    }

    Unmap();
}

void VulkanBuffer::Copy(const std::shared_ptr<Buffer>& buffer)
{
    const std::shared_ptr<VulkanBuffer> vkBuffer = std::static_pointer_cast<VulkanBuffer>(buffer);
    device->CopyBuffer(vkBuffer->GetBuffer(), m_Buffer, vkBuffer->GetBufferSize());
}

/**
 * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
 *
 * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
 * buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the buffer mapping call
 */
void VulkanBuffer::Map(const size_t size, const size_t offset)
{
    assert(m_Buffer && m_Memory && "Called map on buffer before create");
    if (vkMapMemory(device->GetDevice(), m_Memory, offset, size, 0, &m_Mapped))
    {
        FATAL_ERROR("Failed to map buffer!");
    }
}

/**
 * Unmap a mapped memory range
 *
 * @note Does not return a result as vkUnmapMemory can't fail
 */
void VulkanBuffer::Unmap()
{
    if (m_Mapped)
    {
        vkUnmapMemory(device->GetDevice(), m_Memory);
        m_Mapped = nullptr;
    }
}

/**
 * Flush a memory range of the buffer to make it visible to the device
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
 * complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the flush call
 */
VkResult VulkanBuffer::Flush(const VkDeviceSize size, const VkDeviceSize offset) const
{
    VkMappedMemoryRange mappedRange{};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = m_Memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    return vkFlushMappedMemoryRanges(device->GetDevice(), 1, &mappedRange);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
 * the complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the invalidate call
 */
VkResult VulkanBuffer::Invalidate(const VkDeviceSize size, const VkDeviceSize offset) const
{
    VkMappedMemoryRange mappedRange{};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = m_Memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    return vkInvalidateMappedMemoryRanges(device->GetDevice(), 1, &mappedRange);
}

/**
 * Create a buffer info descriptor
 *
 * @param size (Optional) Size of the memory range of the descriptor
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkDescriptorBufferInfo of specified offset and range
 */
VkDescriptorBufferInfo VulkanBuffer::DescriptorInfo(const VkDeviceSize size, const VkDeviceSize offset) const
{
    return VkDescriptorBufferInfo{
        m_Buffer,
        offset,
        size,
    };
}

/**
 * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
 *
 * @param data Pointer to the data to copy
 * @param index Used in offset calculation
 *
 */
void VulkanBuffer::WriteToIndex(void *data, const int index)
{
  WriteToBuffer(data, m_InstanceSize, index * m_AlignmentSize);
}

/**
 *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
 *
 * @param index Used in offset calculation
 *
 */
VkResult VulkanBuffer::FlushIndex(const int index) const { return Flush(m_AlignmentSize, index * m_AlignmentSize); }

/**
 * Create a buffer info descriptor
 *
 * @param index Specifies the region given by index * alignmentSize
 *
 * @return VkDescriptorBufferInfo for instance at index
 */
VkDescriptorBufferInfo VulkanBuffer::DescriptorInfoForIndex(const int index) const
{
  return DescriptorInfo(m_AlignmentSize, index * m_AlignmentSize);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param index Specifies the region to invalidate: index * alignmentSize
 *
 * @return VkResult of the invalidate call
 */
VkResult VulkanBuffer::InvalidateIndex(const int index) const
{
  return Invalidate(m_AlignmentSize, index * m_AlignmentSize);
}