#pragma once

#include "../Core/Core.h"
#include "../Graphics/Buffer.h"

#include <vma/vk_mem_alloc.h>

namespace Pengine::Vk
{

    class PENGINE_API VulkanBuffer final : public Buffer
    {
    public:
        static std::shared_ptr<VulkanBuffer> Create(
            size_t instanceSize,
            uint32_t instanceCount,
            Usage usage,
            MemoryType memoryType);

    	static std::shared_ptr<VulkanBuffer> CreateStagingBuffer(
    		VkDeviceSize instanceSize,
			uint32_t instanceCount);

        static VkBufferUsageFlagBits ConvertUsage(Usage usage);

        static Usage ConvertUsage(VkBufferUsageFlagBits usage);

        VulkanBuffer(
            VkDeviceSize instanceSize,
			uint32_t instanceCount,
			VkBufferUsageFlags bufferUsageFlags,
			VmaMemoryUsage memoryUsage,
			VmaAllocationCreateFlags memoryFlags,
			MemoryType memoryType,
            VkDeviceSize minOffsetAlignment = 1);
        ~VulkanBuffer() override;
        VulkanBuffer(const VkBuffer&) = delete;
    	VulkanBuffer(VkBuffer&&) = delete;
        VulkanBuffer& operator=(const VkBuffer&) = delete;
    	VulkanBuffer& operator=(VkBuffer&&) = delete;

    	[[nodiscard]] virtual void* GetData() const override;

        virtual void WriteToBuffer(void* data, size_t size,
            size_t offset = 0) override;

        virtual void Copy(
        	const std::shared_ptr<Buffer>& buffer,
        	size_t dstOffset) override;

        [[nodiscard]] virtual size_t GetSize() const override { return m_BufferSize; }

        [[nodiscard]] virtual uint32_t GetInstanceCount() const override { return m_InstanceCount; }

        [[nodiscard]] virtual size_t GetInstanceSize() const override { return m_InstanceSize; }

    	[[nodiscard]] VkDescriptorBufferInfo DescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE,
            VkDeviceSize offset = 0) const;

        [[nodiscard]] VkBuffer GetBuffer() const { return m_Buffer; }
        [[nodiscard]] VkDeviceSize GetAlignmentSize() const { return m_AlignmentSize; }
        [[nodiscard]] VkBufferUsageFlags GetUsageFlags() const { return m_UsageFlags; }
        [[nodiscard]] VmaMemoryUsage GetMemoryUsage() const { return m_MemoryUsage; }
        [[nodiscard]] VmaAllocationCreateFlags GetMemoryFlags() const { return m_MemoryFlags; }

    private:
        static VkDeviceSize GetAlignment(
            VkDeviceSize instanceSize,
            VkDeviceSize minOffsetAlignment);

        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VmaAllocation m_VmaAllocation = VK_NULL_HANDLE;
    	VmaAllocationInfo m_VmaAllocationInfo{};

        VkDeviceSize m_BufferSize;
        uint32_t m_InstanceCount;
        VkDeviceSize m_InstanceSize;
        VkDeviceSize m_AlignmentSize;
        VkBufferUsageFlags m_UsageFlags;
    	VmaMemoryUsage m_MemoryUsage;
    	VmaAllocationCreateFlags m_MemoryFlags;
    };

}