#include "VulkanTexture.h"

#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanDescriptors.h"

#include "../Core/Logger.h"

#include <imgui/backends/imgui_impl_vulkan.h>

using namespace Pengine;
using namespace Vk;

VulkanTexture::VulkanTexture(CreateInfo textureCreateInfo)
    : Texture(textureCreateInfo)
{
    VkDeviceSize imageSize = m_Size.x * m_Size.y * textureCreateInfo.channels;

    VkFormat format = ConvertFormat(m_Format);
    VkImageAspectFlagBits aspectMask = ConvertAspectMask(m_AspectMask);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(m_Size.x);
    imageInfo.extent.height = static_cast<uint32_t>(m_Size.y);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = m_MipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional

    for (Usage usage : textureCreateInfo.usage)
    {
        imageInfo.usage |= VulkanTexture::ConvertUsage(usage);
    }

    device->CreateImageWithInfo(
        imageInfo,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_Image,
        m_ImageMemory);

    if (m_Data)
    {
        std::shared_ptr<Buffer> stagingBuffer = Buffer::Create(
            textureCreateInfo.channels,
            m_Size.x * m_Size.y,
            std::vector<Buffer::Usage>{ Buffer::Usage::TRANSFER_SRC }
        );

        stagingBuffer->WriteToBuffer((void*)m_Data);

        TransitionToWrite();

        device->CopyBufferToImage(
            std::static_pointer_cast<VulkanBuffer>(stagingBuffer)->GetBuffer(),
            m_Image,
            static_cast<uint32_t>(m_Size.x),
            static_cast<uint32_t>(m_Size.y),
            1);

        if (m_MipLevels > 1)
        {
            GenerateMipMaps();
        }
        else
        {
            TransitionToRead();
        }
    }

    m_View = CreateImageView(m_Image, format, aspectMask, m_MipLevels);
    m_Sampler = CreateSampler(m_MipLevels);

    std::unique_ptr<VulkanDescriptorSetLayout> setLayout = VulkanDescriptorSetLayout::Builder()
        .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build();

    VulkanDescriptorWriter(*setLayout, *descriptorPool)
        .WriteImage(0, &GetDescriptorInfo())
        .Build(m_DescriptorSet);
}

VulkanTexture::~VulkanTexture()
{
    vkDeviceWaitIdle(device->GetDevice());

    vkDestroySampler(device->GetDevice(), m_Sampler, nullptr);
    vkDestroyImageView(device->GetDevice(), m_View, nullptr);
    vkDestroyImage(device->GetDevice(), m_Image, nullptr);
    vkFreeMemory(device->GetDevice(), m_ImageMemory, nullptr);

    descriptorPool->FreeDescriptors(std::vector<VkDescriptorSet>{ m_DescriptorSet });
}

VkDescriptorImageInfo VulkanTexture::GetDescriptorInfo()
{
    VkDescriptorImageInfo descriptorImageInfo{};
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorImageInfo.imageView = m_View;
    descriptorImageInfo.sampler = m_Sampler;
    
    return descriptorImageInfo;
}

VkImageView VulkanTexture::CreateImageView(VkImage image, VkFormat format,
    VkImageAspectFlagBits aspectMask, int mipLevels)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        FATAL_ERROR("Failed to create texture image view!")
    }

    return imageView;
}

VkSampler VulkanTexture::CreateSampler(uint32_t mipLevels)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels - 1);

    VkSampler sampler;
    if (vkCreateSampler(device->GetDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
    {
        FATAL_ERROR("Failed to create texture sampler!")
    }

    return sampler;
}

VkFormat VulkanTexture::ConvertFormat(Format format)
{
    switch (format)
    {
    case Pengine::Texture::Format::R8G8B8A8_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case Pengine::Texture::Format::B8G8R8A8_UNORM:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case Pengine::Texture::Format::D32_SFLOAT:
        return VK_FORMAT_D32_SFLOAT;
    case Pengine::Texture::Format::D32_SFLOAT_S8_UINT:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case Pengine::Texture::Format::D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case Pengine::Texture::Format::R32_SFLOAT:
        return VK_FORMAT_R32_SFLOAT;
    case Pengine::Texture::Format::R32G32_SFLOAT:
        return VK_FORMAT_R32G32_SFLOAT;
    case Pengine::Texture::Format::R32G32B32_SFLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case Pengine::Texture::Format::R32G32B32A32_SFLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case Pengine::Texture::Format::R16_SFLOAT:
        return VK_FORMAT_R16_SFLOAT;
    case Pengine::Texture::Format::R16G16_SFLOAT:
        return VK_FORMAT_R16G16_SFLOAT;
    case Pengine::Texture::Format::R16G16B16_SFLOAT:
        return VK_FORMAT_R16G16B16_SFLOAT;
    case Pengine::Texture::Format::R16G16B16A16_SFLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;      
    }

    FATAL_ERROR("Failed to convert format!")
}

Texture::Format VulkanTexture::ConvertFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R8G8B8A8_SRGB:
        return Pengine::Texture::Format::R8G8B8A8_SRGB;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return Pengine::Texture::Format::B8G8R8A8_UNORM;
    case VK_FORMAT_D32_SFLOAT:
        return Pengine::Texture::Format::D32_SFLOAT;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return Pengine::Texture::Format::D32_SFLOAT_S8_UINT;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return Pengine::Texture::Format::D24_UNORM_S8_UINT;
    case VK_FORMAT_R32_SFLOAT:
        return Pengine::Texture::Format::R32_SFLOAT;
    case VK_FORMAT_R32G32_SFLOAT:
        return Pengine::Texture::Format::R32G32_SFLOAT;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return Pengine::Texture::Format::R32G32B32_SFLOAT;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return Pengine::Texture::Format::R32G32B32A32_SFLOAT;
    case VK_FORMAT_R16_SFLOAT:
        return Pengine::Texture::Format::R16_SFLOAT;
    case VK_FORMAT_R16G16_SFLOAT:
        return Pengine::Texture::Format::R16G16_SFLOAT;
    case VK_FORMAT_R16G16B16_SFLOAT:
        return Pengine::Texture::Format::R16G16B16_SFLOAT;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return Pengine::Texture::Format::R16G16B16A16_SFLOAT;
    }

    FATAL_ERROR("Failed to convert format!")
}

VkImageAspectFlagBits VulkanTexture::ConvertAspectMask(AspectMask aspectMask)
{
    switch (aspectMask)
    {
    case Pengine::Texture::AspectMask::COLOR:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    case Pengine::Texture::AspectMask::DEPTH:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    FATAL_ERROR("Failed to convert aspect mask!")
}

Texture::AspectMask VulkanTexture::ConvertAspectMask(VkImageAspectFlagBits aspectMask)
{
    switch (aspectMask)
    {
    case VK_IMAGE_ASPECT_COLOR_BIT:
        return Pengine::Texture::AspectMask::COLOR;
    case VK_IMAGE_ASPECT_DEPTH_BIT:
        return Pengine::Texture::AspectMask::DEPTH;
    }

    FATAL_ERROR("Failed to convert aspect mask!")
}

VkImageLayout VulkanTexture::ConvertLayout(Layout layout)
{
    switch (layout)
    {
    case Pengine::Texture::Layout::UNDEFINED:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case Pengine::Texture::Layout::SHADER_READ_ONLY_OPTIMAL:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case Pengine::Texture::Layout::COLOR_ATTACHMENT_OPTIMAL:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case Pengine::Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case Pengine::Texture::Layout::DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }

    FATAL_ERROR("Failed to convert texture layout!")
}

Texture::Layout VulkanTexture::ConvertLayout(VkImageLayout layout)
{
    switch (layout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return Pengine::Texture::Layout::UNDEFINED;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return Pengine::Texture::Layout::SHADER_READ_ONLY_OPTIMAL;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return Pengine::Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return Pengine::Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        return Pengine::Texture::Layout::DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }

    FATAL_ERROR("Failed to convert texture layout!")
}

VkImageUsageFlagBits VulkanTexture::ConvertUsage(Usage usage)
{
    switch (usage)
    {
    case Pengine::Texture::Usage::SAMPLED:
        return VK_IMAGE_USAGE_SAMPLED_BIT;
    case Pengine::Texture::Usage::TRANSFER_DST:
        return VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    case Pengine::Texture::Usage::TRANSFER_SRC:
        return VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    case Pengine::Texture::Usage::DEPTH_STENCIL_ATTACHMENT:
        return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    case Pengine::Texture::Usage::COLOR_ATTACHMENT:
        return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    FATAL_ERROR("Failed to convert usage!")
}

Texture::Usage VulkanTexture::ConvertUsage(VkImageUsageFlagBits usage)
{
    switch (usage)
    {
    case VK_IMAGE_USAGE_SAMPLED_BIT:
        return Pengine::Texture::Usage::SAMPLED;
    case VK_IMAGE_USAGE_TRANSFER_DST_BIT:
        return Pengine::Texture::Usage::TRANSFER_DST;
    case VK_IMAGE_USAGE_TRANSFER_SRC_BIT:
        return Pengine::Texture::Usage::TRANSFER_SRC;
    case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:
        return Pengine::Texture::Usage::DEPTH_STENCIL_ATTACHMENT;
    case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:
        return Pengine::Texture::Usage::COLOR_ATTACHMENT;
    }

    FATAL_ERROR("Failed to convert usage!")
}

void VulkanTexture::GenerateMipMaps()
{
    Vk::device->GenerateMipMaps(m_Image, VulkanTexture::ConvertFormat(m_Format), m_Size.x, m_Size.y, m_MipLevels);
}

void VulkanTexture::TransitionToWrite()
{
    device->TransitionImageLayout(
        m_Image,
        VulkanTexture::ConvertFormat(m_Format),
        VulkanTexture::ConvertAspectMask(m_AspectMask),
        m_Layout,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        m_MipLevels);

    m_Layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
}

void VulkanTexture::TransitionToRead()
{
    device->TransitionImageLayout(
        m_Image,
        VulkanTexture::ConvertFormat(m_Format),
        VulkanTexture::ConvertAspectMask(m_AspectMask),
        m_Layout,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        m_MipLevels);

    m_Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void VulkanTexture::TransitionToColorAttachment()
{
    device->TransitionImageLayout(
        m_Image,
        VulkanTexture::ConvertFormat(m_Format),
        VulkanTexture::ConvertAspectMask(m_AspectMask),
        m_Layout,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        m_MipLevels);

    m_Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}
