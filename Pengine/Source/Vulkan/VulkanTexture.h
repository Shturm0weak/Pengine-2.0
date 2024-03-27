#pragma once

#include "../Core/Core.h"
#include "../Graphics/Texture.h"

#include <vma/vk_mem_alloc.h>

namespace Pengine::Vk
{

    class PENGINE_API VulkanTexture final : public Texture
    {
    public:
        explicit VulkanTexture(const CreateInfo& textureCreateInfo);
        virtual ~VulkanTexture() override;
        VulkanTexture(const VulkanTexture&) = delete;
    	VulkanTexture(VulkanTexture&&) = delete;
        VulkanTexture& operator=(const VulkanTexture&) = delete;
    	VulkanTexture& operator=(VulkanTexture&&) = delete;

        VkDescriptorImageInfo GetDescriptorInfo();

        static VkImageView CreateImageView(
        	VkImage image,
        	VkFormat format,
            VkImageAspectFlagBits aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            uint32_t mipLevels = 1);

        static VkSampler CreateSampler(uint32_t mipLevels = 1);

        static VkFormat ConvertFormat(Format format);

        static Format ConvertFormat(VkFormat format);

        static VkImageAspectFlagBits ConvertAspectMask(AspectMask aspectMask);

        static AspectMask ConvertAspectMask(VkImageAspectFlagBits aspectMask);

        static VkImageLayout ConvertLayout(Layout layout);

        static Layout ConvertLayout(VkImageLayout layout);

        static VkImageUsageFlagBits ConvertUsage(Usage usage);

        static Usage ConvertUsage(VkImageUsageFlagBits usage);

        [[nodiscard]] virtual void* GetId() const override { return (void*)m_DescriptorSet; }

        virtual void GenerateMipMaps() override;

        [[nodiscard]] VkImageView GetImageView() const { return m_View; }

        [[nodiscard]] VkImage GetImage() const { return m_Image; }

        [[nodiscard]] VkSampler GetSampler() const { return m_Sampler; }

        [[nodiscard]] VkImageLayout GetLayout() const { return m_Layout; }

        void TransitionToWrite();

        void TransitionToRead();

        void TransitionToColorAttachment();

    private:
        VkSampler m_Sampler{};
        VkImage m_Image{};
        VkImageLayout m_ImageLayout{};
    	VmaAllocation m_VmaAllocation = VK_NULL_HANDLE;
    	VmaAllocationInfo m_VmaAllocationInfo{};
        VkImageView m_View{};
        VkDescriptorSet m_DescriptorSet{};
        VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

}