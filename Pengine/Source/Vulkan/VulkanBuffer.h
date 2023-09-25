#pragma once

#include "../Core/Core.h"
#include "../Graphics/Buffer.h"

namespace Pengine
{

    namespace Vk
    {

        class PENGINE_API VulkanBuffer : public Buffer
        {
        public:
            static std::shared_ptr<VulkanBuffer> Create(uint64_t instanceSize,
                uint32_t instanceCount, std::vector<Usage> usage);

            static VkBufferUsageFlagBits ConvertUsage(Usage usage);

            static Usage ConvertUsage(VkBufferUsageFlagBits usage);

            VulkanBuffer(
                VkDeviceSize instanceSize,
                uint32_t instanceCount,
                VkBufferUsageFlags usageFlags,
                VkMemoryPropertyFlags memoryPropertyFlags,
                VkDeviceSize minOffsetAlignment = 1);
            ~VulkanBuffer();
            VulkanBuffer(const VkBuffer&) = delete;
            VulkanBuffer& operator=(const VkBuffer&) = delete;

            virtual void* GetData() override;

            virtual void WriteToBuffer(void* data, size_t size = -1,
                size_t offset = 0) override;

            virtual void Copy(std::shared_ptr<Buffer> buffer) override;

            virtual uint32_t GetInstanceCount() const { return m_InstanceCount; }

            virtual size_t GetInstanceSize() const { return m_InstanceSize; }

            virtual void Map(size_t size = -1, size_t offset = 0) override;

            virtual void Unmap() override;

            VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
            VkDescriptorBufferInfo DescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE,
                VkDeviceSize offset = 0);
            VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

            void WriteToIndex(void* data, int index);
            VkResult FlushIndex(int index);
            VkDescriptorBufferInfo DescriptorInfoForIndex(int index);
            VkResult InvalidateIndex(int index);

            VkBuffer GetBuffer() const { return m_Buffer; }
            void* GetMappedMemory() const { return m_Mapped; }
            VkDeviceSize GetAlignmentSize() const { return m_AlignmentSize; }
            VkBufferUsageFlags GetUsageFlags() const { return m_UsageFlags; }
            VkMemoryPropertyFlags GetMemoryPropertyFlags() const { return m_MemoryPropertyFlags; }
            VkDeviceSize GetBufferSize() const { return m_BufferSize; }

        private:
            static VkDeviceSize GetAlignment(VkDeviceSize instanceSize,
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

}