#pragma once

#include "../Core/Core.h"
#include "../Graphics/Texture.h"

#include <vulkan/vulkan.h>

namespace Pengine
{

    namespace Vk
    {

        class PENGINE_API VulkanTexture : public Texture
        {
        public:
            VulkanTexture(CreateInfo textureCreateInfo);
            ~VulkanTexture();
            VulkanTexture(const VulkanTexture&) = delete;
            VulkanTexture& operator=(const VulkanTexture&) = delete;

            VkDescriptorImageInfo GetDescriptorInfo();

            static VkImageView CreateImageView(VkImage image, VkFormat format,
                VkImageAspectFlagBits aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, int mipLevels = 1);

            static VkSampler CreateSampler(uint32_t mipLevels = 1);

            static VkFormat ConvertFormat(Format format);

            static Format ConvertFormat(VkFormat format);

            static VkImageAspectFlagBits ConvertAspectMask(AspectMask aspectMask);

            static AspectMask ConvertAspectMask(VkImageAspectFlagBits aspectMask);

            static VkImageLayout ConvertLayout(Layout layout);

            static Layout ConvertLayout(VkImageLayout layout);

            static VkImageUsageFlagBits ConvertUsage(Usage usage);

            static Usage ConvertUsage(VkImageUsageFlagBits usage);

            virtual void* GetId() const override { return (void*)m_DescriptorSet; }

            virtual void GenerateMipMaps() override;

            VkImageView GetImageView() const { return m_View; }

            VkImage GetImage() const { return m_Image; }

            VkSampler GetSampler() const { return m_Sampler; }

            VkImageLayout GetLayout() const { return m_Layout; }

            void TransitionToWrite();

            void TransitionToRead();

            void TransitionToColorAttachment();

        private:
            VkSampler m_Sampler;
            VkImage m_Image;
            VkImageLayout m_ImageLayout;
            VkDeviceMemory m_ImageMemory;
            VkImageView m_View;
            VkDescriptorSet m_DescriptorSet;
            VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
        };

    }

}