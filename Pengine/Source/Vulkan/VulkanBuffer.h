#pragma once

#include "../Core/Core.h"
#include "../Graphics/Buffer.h"

#include <vulkan/vulkan.h>

namespace Pengine::Vk
{

    class PENGINE_API VulkanBuffer final : public Buffer
    {
    public:
        static std::shared_ptr<VulkanBuffer> Create(
            size_t instanceSize,
            uint32_t instanceCount,
            const std::vector<Usage>& usage);

        static VkBufferUsageFlagBits ConvertUsage(Usage usage);

        static Usage ConvertUsage(VkBufferUsageFlagBits usage);

        VulkanBuffer(
            VkDeviceSize instanceSize,
            uint32_t instanceCount,
            VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memoryPropertyFlags,
            VkDeviceSize minOffsetAlignment = 1);
        ~VulkanBuffer() override;
        VulkanBuffer(const VkBuffer&) = delete;
    	VulkanBuffer(VkBuffer&&) = delete;
        VulkanBuffer& operator=(const VkBuffer&) = delete;
    	VulkanBuffer& operator=(VkBuffer&&) = delete;

        [[nodiscard]] virtual void* GetData() override;

        virtual void WriteToBuffer(void* data, size_t size = -1,
            size_t offset = 0) override;

        virtual void Copy(const std::shared_ptr<Buffer>& buffer) override;

        [[nodiscard]] virtual uint32_t GetInstanceCount() const override { return m_InstanceCount; }

        [[nodiscard]] virtual size_t GetInstanceSize() const override { return m_InstanceSize; }

        virtual void Map(size_t size = -1, size_t offset = 0) override;

        virtual void Unmap() override;

        [[nodiscard]] VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
    	[[nodiscard]] VkDescriptorBufferInfo DescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE,
            VkDeviceSize offset = 0) const;
        [[nodiscard]] VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

        void WriteToIndex(void* data, int index);
    	[[nodiscard]] VkResult FlushIndex(int index) const;
        [[nodiscard]] VkDescriptorBufferInfo DescriptorInfoForIndex(int index) const;
        [[nodiscard]] VkResult InvalidateIndex(int index) const;

        [[nodiscard]] VkBuffer GetBuffer() const { return m_Buffer; }
        [[nodiscard]] void* GetMappedMemory() const { return m_Mapped; }
        [[nodiscard]] VkDeviceSize GetAlignmentSize() const { return m_AlignmentSize; }
        [[nodiscard]] VkBufferUsageFlags GetUsageFlags() const { return m_UsageFlags; }
        [[nodiscard]] VkMemoryPropertyFlags GetMemoryPropertyFlags() const { return m_MemoryPropertyFlags; }
        [[nodiscard]] VkDeviceSize GetBufferSize() const { return m_BufferSize; }

    private:
        static VkDeviceSize GetAlignment(
            VkDeviceSize instanceSize,
            VkDeviceSize minOffsetAlignment);

        void* m_Mapped = nullptr;
        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;

        VkDeviceSize m_BufferSize;
        uint32_t m_InstanceCount;
        VkDeviceSize m_InstanceSize;
        VkDeviceSize m_AlignmentSize;
        VkBufferUsageFlags m_UsageFlags;
        VkMemoryPropertyFlags m_MemoryPropertyFlags;
    };

}