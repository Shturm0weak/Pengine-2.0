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

    if (!m_Data.empty())
    {
        std::shared_ptr<Buffer> stagingBuffer = Buffer::Create(
            textureCreateInfo.channels,
            m_Size.x * m_Size.y,
            std::vector<Buffer::Usage>{ Buffer::Usage::TRANSFER_SRC }
        );

        stagingBuffer->WriteToBuffer((void*)m_Data.data());

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
    case Pengine::Texture::Format::UNDEFINED:
        return VK_FORMAT_UNDEFINED;
    case Pengine::Texture::Format::R4G4_UNORM_PACK8:
        return VK_FORMAT_R4G4_UNORM_PACK8;
    case Pengine::Texture::Format::R4G4B4A4_UNORM_PACK16:
        return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
    case Pengine::Texture::Format::B4G4R4A4_UNORM_PACK16:
        return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
    case Pengine::Texture::Format::R5G6B5_UNORM_PACK16:
        return VK_FORMAT_R5G6B5_UNORM_PACK16;
    case Pengine::Texture::Format::B5G6R5_UNORM_PACK16:
        return VK_FORMAT_B5G6R5_UNORM_PACK16;
    case Pengine::Texture::Format::R5G5B5A1_UNORM_PACK16:
        return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
    case Pengine::Texture::Format::B5G5R5A1_UNORM_PACK16:
        return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
    case Pengine::Texture::Format::A1R5G5B5_UNORM_PACK16:
        return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
    case Pengine::Texture::Format::R8_UNORM:
        return VK_FORMAT_R8_UNORM;
    case Pengine::Texture::Format::R8_SNORM:
        return VK_FORMAT_R8_SNORM;
    case Pengine::Texture::Format::R8_USCALED:
        return VK_FORMAT_R8_USCALED;
    case Pengine::Texture::Format::R8_SSCALED:
        return VK_FORMAT_R8_SSCALED;
    case Pengine::Texture::Format::R8_UINT:
        return VK_FORMAT_R8_UINT;
    case Pengine::Texture::Format::R8_SINT:
        return VK_FORMAT_R8_SINT;
    case Pengine::Texture::Format::R8_SRGB:
        return VK_FORMAT_R8_SRGB;
    case Pengine::Texture::Format::R8G8_UNORM:
        return VK_FORMAT_R8G8_UNORM;
    case Pengine::Texture::Format::R8G8_SNORM:
        return VK_FORMAT_R8G8_SNORM;
    case Pengine::Texture::Format::R8G8_USCALED:
        return VK_FORMAT_R8G8_USCALED;
    case Pengine::Texture::Format::R8G8_SSCALED:
        return VK_FORMAT_R8G8_SSCALED;
    case Pengine::Texture::Format::R8G8_UINT:
        return VK_FORMAT_R8G8_UINT;
    case Pengine::Texture::Format::R8G8_SINT:
        return VK_FORMAT_R8G8_SINT;
    case Pengine::Texture::Format::R8G8_SRGB:
        return VK_FORMAT_R8G8_SRGB;
    case Pengine::Texture::Format::R8G8B8_UNORM:
        return VK_FORMAT_R8G8B8_UNORM;
    case Pengine::Texture::Format::R8G8B8_SNORM:
        return VK_FORMAT_R8G8B8_SNORM;
    case Pengine::Texture::Format::R8G8B8_USCALED:
        return VK_FORMAT_R8G8B8_USCALED;
    case Pengine::Texture::Format::R8G8B8_SSCALED:
        return VK_FORMAT_R8G8B8_SSCALED;
    case Pengine::Texture::Format::R8G8B8_UINT:
        return VK_FORMAT_R8G8B8_UINT;
    case Pengine::Texture::Format::R8G8B8_SINT:
        return VK_FORMAT_R8G8B8_SINT;
    case Pengine::Texture::Format::R8G8B8_SRGB:
        return VK_FORMAT_R8G8B8_SRGB;
    case Pengine::Texture::Format::B8G8R8_UNORM:
        return VK_FORMAT_B8G8R8_UNORM;
    case Pengine::Texture::Format::B8G8R8_SNORM:
        return VK_FORMAT_B8G8R8_SNORM;
    case Pengine::Texture::Format::B8G8R8_USCALED:
        return VK_FORMAT_B8G8R8_USCALED;
    case Pengine::Texture::Format::B8G8R8_SSCALED:
        return VK_FORMAT_B8G8R8_SSCALED;
    case Pengine::Texture::Format::B8G8R8_UINT:
        return VK_FORMAT_B8G8R8_UINT;
    case Pengine::Texture::Format::B8G8R8_SINT:
        return VK_FORMAT_B8G8R8_SINT;
    case Pengine::Texture::Format::B8G8R8_SRGB:
        return VK_FORMAT_B8G8R8_SRGB;
    case Pengine::Texture::Format::R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case Pengine::Texture::Format::R8G8B8A8_SNORM:
        return VK_FORMAT_R8G8B8A8_SNORM;
    case Pengine::Texture::Format::R8G8B8A8_USCALED:
        return VK_FORMAT_R8G8B8A8_USCALED;
    case Pengine::Texture::Format::R8G8B8A8_SSCALED:
        return VK_FORMAT_R8G8B8A8_SSCALED;
    case Pengine::Texture::Format::R8G8B8A8_UINT:
        return VK_FORMAT_R8G8B8A8_UINT;
    case Pengine::Texture::Format::R8G8B8A8_SINT:
        return VK_FORMAT_R8G8B8A8_SINT;
    case Pengine::Texture::Format::R8G8B8A8_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case Pengine::Texture::Format::B8G8R8A8_UNORM:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case Pengine::Texture::Format::B8G8R8A8_SNORM:
        return VK_FORMAT_B8G8R8A8_SNORM;
    case Pengine::Texture::Format::B8G8R8A8_USCALED:
        return VK_FORMAT_B8G8R8A8_USCALED;
    case Pengine::Texture::Format::B8G8R8A8_SSCALED:
        return VK_FORMAT_B8G8R8A8_SSCALED;
    case Pengine::Texture::Format::B8G8R8A8_UINT:
        return VK_FORMAT_B8G8R8A8_UINT;
    case Pengine::Texture::Format::B8G8R8A8_SINT:
        return VK_FORMAT_B8G8R8A8_SINT;
    case Pengine::Texture::Format::B8G8R8A8_SRGB:
        return VK_FORMAT_B8G8R8A8_SRGB;
    case Pengine::Texture::Format::A8B8G8R8_UNORM_PACK32:
        return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
    case Pengine::Texture::Format::A8B8G8R8_SNORM_PACK32:
        return VK_FORMAT_A8B8G8R8_SNORM_PACK32;
    case Pengine::Texture::Format::A8B8G8R8_USCALED_PACK32:
        return VK_FORMAT_A8B8G8R8_USCALED_PACK32;
    case Pengine::Texture::Format::A8B8G8R8_SSCALED_PACK32:
        return VK_FORMAT_A8B8G8R8_SSCALED_PACK32;
    case Pengine::Texture::Format::A8B8G8R8_UINT_PACK32:
        return VK_FORMAT_A8B8G8R8_UINT_PACK32;
    case Pengine::Texture::Format::A8B8G8R8_SINT_PACK32:
        return VK_FORMAT_A8B8G8R8_SINT_PACK32;
    case Pengine::Texture::Format::A8B8G8R8_SRGB_PACK32:
        return VK_FORMAT_A8B8G8R8_SRGB_PACK32;
    case Pengine::Texture::Format::A2R10G10B10_UNORM_PACK32:
        return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    case Pengine::Texture::Format::A2R10G10B10_SNORM_PACK32:
        return VK_FORMAT_A2R10G10B10_SNORM_PACK32;
    case Pengine::Texture::Format::A2R10G10B10_USCALED_PACK32:
        return VK_FORMAT_A2R10G10B10_USCALED_PACK32;
    case Pengine::Texture::Format::A2R10G10B10_SSCALED_PACK32:
        return VK_FORMAT_A2R10G10B10_SSCALED_PACK32;
    case Pengine::Texture::Format::A2R10G10B10_UINT_PACK32:
        return VK_FORMAT_A2R10G10B10_UINT_PACK32;
    case Pengine::Texture::Format::A2R10G10B10_SINT_PACK32:
        return VK_FORMAT_A2R10G10B10_SINT_PACK32;
    case Pengine::Texture::Format::A2B10G10R10_UNORM_PACK32:
        return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    case Pengine::Texture::Format::A2B10G10R10_SNORM_PACK32:
        return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
    case Pengine::Texture::Format::A2B10G10R10_USCALED_PACK32:
        return VK_FORMAT_A2B10G10R10_USCALED_PACK32;
    case Pengine::Texture::Format::A2B10G10R10_SSCALED_PACK32:
        return VK_FORMAT_A2B10G10R10_SSCALED_PACK32;
    case Pengine::Texture::Format::A2B10G10R10_UINT_PACK32:
        return VK_FORMAT_A2B10G10R10_UINT_PACK32;
    case Pengine::Texture::Format::A2B10G10R10_SINT_PACK32:
        return VK_FORMAT_A2B10G10R10_SINT_PACK32;
    case Pengine::Texture::Format::R16_UNORM:
        return VK_FORMAT_R16_UNORM;
    case Pengine::Texture::Format::R16_SNORM:
        return VK_FORMAT_R16_SNORM;
    case Pengine::Texture::Format::R16_USCALED:
        return VK_FORMAT_R16_USCALED;
    case Pengine::Texture::Format::R16_SSCALED:
        return VK_FORMAT_R16_SSCALED;
    case Pengine::Texture::Format::R16_UINT:
        return VK_FORMAT_R16_UINT;
    case Pengine::Texture::Format::R16_SINT:
        return VK_FORMAT_R16_SINT;
    case Pengine::Texture::Format::R16_SFLOAT:
        return VK_FORMAT_R16_SFLOAT;
    case Pengine::Texture::Format::R16G16_UNORM:
        return VK_FORMAT_R16G16_UNORM;
    case Pengine::Texture::Format::R16G16_SNORM:
        return VK_FORMAT_R16G16_SNORM;
    case Pengine::Texture::Format::R16G16_USCALED:
        return VK_FORMAT_R16G16_USCALED;
    case Pengine::Texture::Format::R16G16_SSCALED:
        return VK_FORMAT_R16G16_SSCALED;
    case Pengine::Texture::Format::R16G16_UINT:
        return VK_FORMAT_R16G16_UINT;
    case Pengine::Texture::Format::R16G16_SINT:
        return VK_FORMAT_R16G16_SINT;
    case Pengine::Texture::Format::R16G16_SFLOAT:
        return VK_FORMAT_R16G16_SFLOAT;
    case Pengine::Texture::Format::R16G16B16_UNORM:
        return VK_FORMAT_R16G16B16_UNORM;
    case Pengine::Texture::Format::R16G16B16_SNORM:
        return VK_FORMAT_R16G16B16_SNORM;
    case Pengine::Texture::Format::R16G16B16_USCALED:
        return VK_FORMAT_R16G16B16_USCALED;
    case Pengine::Texture::Format::R16G16B16_SSCALED:
        return VK_FORMAT_R16G16B16_SSCALED;
    case Pengine::Texture::Format::R16G16B16_UINT:
        return VK_FORMAT_R16G16B16_UINT;
    case Pengine::Texture::Format::R16G16B16_SINT:
        return VK_FORMAT_R16G16B16_SINT;
    case Pengine::Texture::Format::R16G16B16_SFLOAT:
        return VK_FORMAT_R16G16B16_SFLOAT;
    case Pengine::Texture::Format::R16G16B16A16_UNORM:
        return VK_FORMAT_R16G16B16A16_UNORM;
    case Pengine::Texture::Format::R16G16B16A16_SNORM:
        return VK_FORMAT_R16G16B16A16_SNORM;
    case Pengine::Texture::Format::R16G16B16A16_USCALED:
        return VK_FORMAT_R16G16B16A16_USCALED;
    case Pengine::Texture::Format::R16G16B16A16_SSCALED:
        return VK_FORMAT_R16G16B16A16_SSCALED;
    case Pengine::Texture::Format::R16G16B16A16_UINT:
        return VK_FORMAT_R16G16B16A16_UINT;
    case Pengine::Texture::Format::R16G16B16A16_SINT:
        return VK_FORMAT_R16G16B16A16_SINT;
    case Pengine::Texture::Format::R16G16B16A16_SFLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case Pengine::Texture::Format::R32_UINT:
        return VK_FORMAT_R32_UINT;
    case Pengine::Texture::Format::R32_SINT:
        return VK_FORMAT_R32_SINT;
    case Pengine::Texture::Format::R32_SFLOAT:
        return VK_FORMAT_R32_SFLOAT;
    case Pengine::Texture::Format::R32G32_UINT:
        return VK_FORMAT_R32G32_UINT;
    case Pengine::Texture::Format::R32G32_SINT:
        return VK_FORMAT_R32G32_SINT;
    case Pengine::Texture::Format::R32G32_SFLOAT:
        return VK_FORMAT_R32G32_SFLOAT;
    case Pengine::Texture::Format::R32G32B32_UINT:
        return VK_FORMAT_R32G32B32_UINT;
    case Pengine::Texture::Format::R32G32B32_SINT:
        return VK_FORMAT_R32G32B32_SINT;
    case Pengine::Texture::Format::R32G32B32_SFLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case Pengine::Texture::Format::R32G32B32A32_UINT:
        return VK_FORMAT_R32G32B32A32_UINT;
    case Pengine::Texture::Format::R32G32B32A32_SINT:
        return VK_FORMAT_R32G32B32A32_SINT;
    case Pengine::Texture::Format::R32G32B32A32_SFLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case Pengine::Texture::Format::R64_UINT:
        return VK_FORMAT_R64_UINT;
    case Pengine::Texture::Format::R64_SINT:
        return VK_FORMAT_R64_SINT;
    case Pengine::Texture::Format::R64_SFLOAT:
        return VK_FORMAT_R64_SFLOAT;
    case Pengine::Texture::Format::R64G64_UINT:
        return VK_FORMAT_R64G64_UINT;
    case Pengine::Texture::Format::R64G64_SINT:
        return VK_FORMAT_R64G64_SINT;
    case Pengine::Texture::Format::R64G64_SFLOAT:
        return VK_FORMAT_R64G64_SFLOAT;
    case Pengine::Texture::Format::R64G64B64_UINT:
        return VK_FORMAT_R64G64B64_UINT;
    case Pengine::Texture::Format::R64G64B64_SINT:
        return VK_FORMAT_R64G64B64_SINT;
    case Pengine::Texture::Format::R64G64B64_SFLOAT:
        return VK_FORMAT_R64G64B64_SFLOAT;
    case Pengine::Texture::Format::R64G64B64A64_UINT:
        return VK_FORMAT_R64G64B64A64_UINT;
    case Pengine::Texture::Format::R64G64B64A64_SINT:
        return VK_FORMAT_R64G64B64A64_SINT;
    case Pengine::Texture::Format::R64G64B64A64_SFLOAT:
        return VK_FORMAT_R64G64B64A64_SFLOAT;
    case Pengine::Texture::Format::B10G11R11_UFLOAT_PACK32:
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case Pengine::Texture::Format::E5B9G9R9_UFLOAT_PACK32:
        return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
    case Pengine::Texture::Format::D16_UNORM:
        return VK_FORMAT_D16_UNORM;
    case Pengine::Texture::Format::X8_D24_UNORM_PACK32:
        return VK_FORMAT_X8_D24_UNORM_PACK32;
    case Pengine::Texture::Format::D32_SFLOAT:
        return VK_FORMAT_D32_SFLOAT;
    case Pengine::Texture::Format::S8_UINT:
        return VK_FORMAT_S8_UINT;
    case Pengine::Texture::Format::D16_UNORM_S8_UINT:
        return VK_FORMAT_D16_UNORM_S8_UINT;
    case Pengine::Texture::Format::D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case Pengine::Texture::Format::D32_SFLOAT_S8_UINT:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case Pengine::Texture::Format::BC1_RGB_UNORM_BLOCK:
        return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    case Pengine::Texture::Format::BC1_RGB_SRGB_BLOCK:
        return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
    case Pengine::Texture::Format::BC1_RGBA_UNORM_BLOCK:
        return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case Pengine::Texture::Format::BC1_RGBA_SRGB_BLOCK:
        return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    case Pengine::Texture::Format::BC2_UNORM_BLOCK:
        return VK_FORMAT_BC2_UNORM_BLOCK;
    case Pengine::Texture::Format::BC2_SRGB_BLOCK:
        return VK_FORMAT_BC2_SRGB_BLOCK;
    case Pengine::Texture::Format::BC3_UNORM_BLOCK:
        return VK_FORMAT_BC3_UNORM_BLOCK;
    case Pengine::Texture::Format::BC3_SRGB_BLOCK:
        return VK_FORMAT_BC3_SRGB_BLOCK;
    case Pengine::Texture::Format::BC4_UNORM_BLOCK:
        return VK_FORMAT_BC4_UNORM_BLOCK;
    case Pengine::Texture::Format::BC4_SNORM_BLOCK:
        return VK_FORMAT_BC4_SNORM_BLOCK;
    case Pengine::Texture::Format::BC5_UNORM_BLOCK:
        return VK_FORMAT_BC5_UNORM_BLOCK;
    case Pengine::Texture::Format::BC5_SNORM_BLOCK:
        return VK_FORMAT_BC5_SNORM_BLOCK;
    case Pengine::Texture::Format::BC6H_UFLOAT_BLOCK:
        return VK_FORMAT_BC6H_UFLOAT_BLOCK;
    case Pengine::Texture::Format::BC6H_SFLOAT_BLOCK:
        return VK_FORMAT_BC6H_SFLOAT_BLOCK;
    case Pengine::Texture::Format::BC7_UNORM_BLOCK:
        return VK_FORMAT_BC7_UNORM_BLOCK;
    case Pengine::Texture::Format::BC7_SRGB_BLOCK:
        return VK_FORMAT_BC7_SRGB_BLOCK;
    case Pengine::Texture::Format::ETC2_R8G8B8_UNORM_BLOCK:
        return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
    case Pengine::Texture::Format::ETC2_R8G8B8_SRGB_BLOCK:
        return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
    case Pengine::Texture::Format::ETC2_R8G8B8A1_UNORM_BLOCK:
        return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
    case Pengine::Texture::Format::ETC2_R8G8B8A1_SRGB_BLOCK:
        return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
    case Pengine::Texture::Format::ETC2_R8G8B8A8_UNORM_BLOCK:
        return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
    case Pengine::Texture::Format::ETC2_R8G8B8A8_SRGB_BLOCK:
        return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
    case Pengine::Texture::Format::EAC_R11_UNORM_BLOCK:
        return VK_FORMAT_EAC_R11_UNORM_BLOCK;
    case Pengine::Texture::Format::EAC_R11_SNORM_BLOCK:
        return VK_FORMAT_EAC_R11_SNORM_BLOCK;
    case Pengine::Texture::Format::EAC_R11G11_UNORM_BLOCK:
        return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
    case Pengine::Texture::Format::EAC_R11G11_SNORM_BLOCK:
        return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_4x4_UNORM_BLOCK:
        return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_4x4_SRGB_BLOCK:
        return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
    case Pengine::Texture::Format::ASTC_5x4_UNORM_BLOCK:
        return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_5x4_SRGB_BLOCK:
        return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
    case Pengine::Texture::Format::ASTC_5x5_UNORM_BLOCK:
        return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_5x5_SRGB_BLOCK:
        return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
    case Pengine::Texture::Format::ASTC_6x5_UNORM_BLOCK:
        return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_6x5_SRGB_BLOCK:
        return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
    case Pengine::Texture::Format::ASTC_6x6_UNORM_BLOCK:
        return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_6x6_SRGB_BLOCK:
        return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
    case Pengine::Texture::Format::ASTC_8x5_UNORM_BLOCK:
        return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_8x5_SRGB_BLOCK:
        return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
    case Pengine::Texture::Format::ASTC_8x6_UNORM_BLOCK:
        return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_8x6_SRGB_BLOCK:
        return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
    case Pengine::Texture::Format::ASTC_8x8_UNORM_BLOCK:
        return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_8x8_SRGB_BLOCK:
        return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
    case Pengine::Texture::Format::ASTC_10x5_UNORM_BLOCK:
        return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_10x5_SRGB_BLOCK:
        return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
    case Pengine::Texture::Format::ASTC_10x6_UNORM_BLOCK:
        return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_10x6_SRGB_BLOCK:
        return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
    case Pengine::Texture::Format::ASTC_10x8_UNORM_BLOCK:
        return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_10x8_SRGB_BLOCK:
        return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
    case Pengine::Texture::Format::ASTC_10x10_UNORM_BLOCK:
        return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_10x10_SRGB_BLOCK:
        return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
    case Pengine::Texture::Format::ASTC_12x10_UNORM_BLOCK:
        return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_12x10_SRGB_BLOCK:
        return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
    case Pengine::Texture::Format::ASTC_12x12_UNORM_BLOCK:
        return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
    case Pengine::Texture::Format::ASTC_12x12_SRGB_BLOCK:
        return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
    case Pengine::Texture::Format::G8B8G8R8_422_UNORM:
        return VK_FORMAT_G8B8G8R8_422_UNORM;
    case Pengine::Texture::Format::B8G8R8G8_422_UNORM:
        return VK_FORMAT_B8G8R8G8_422_UNORM;
    case Pengine::Texture::Format::G8_B8_R8_3PLANE_420_UNORM:
        return VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
    case Pengine::Texture::Format::G8_B8R8_2PLANE_420_UNORM:
        return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
    case Pengine::Texture::Format::G8_B8_R8_3PLANE_422_UNORM:
        return VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM;
    case Pengine::Texture::Format::G8_B8R8_2PLANE_422_UNORM:
        return VK_FORMAT_G8_B8R8_2PLANE_422_UNORM;
    case Pengine::Texture::Format::G8_B8_R8_3PLANE_444_UNORM:
        return VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM;
    case Pengine::Texture::Format::R10X6_UNORM_PACK16:
        return VK_FORMAT_R10X6_UNORM_PACK16;
    case Pengine::Texture::Format::R10X6G10X6_UNORM_2PACK16:
        return VK_FORMAT_R10X6G10X6_UNORM_2PACK16;
    case Pengine::Texture::Format::R10X6G10X6B10X6A10X6_UNORM_4PACK16:
        return VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16;
    case Pengine::Texture::Format::G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
        return VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16;
    case Pengine::Texture::Format::B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
        return VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16;
    case Pengine::Texture::Format::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
        return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16;
    case Pengine::Texture::Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
    case Pengine::Texture::Format::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
        return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16;
    case Pengine::Texture::Format::G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16;
    case Pengine::Texture::Format::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16;
    case Pengine::Texture::Format::R12X4_UNORM_PACK16:
        return VK_FORMAT_R12X4_UNORM_PACK16;
    case Pengine::Texture::Format::R12X4G12X4_UNORM_2PACK16:
        return VK_FORMAT_R12X4G12X4_UNORM_2PACK16;
    case Pengine::Texture::Format::R12X4G12X4B12X4A12X4_UNORM_4PACK16:
        return VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16;
    case Pengine::Texture::Format::G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
        return VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16;
    case Pengine::Texture::Format::B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
        return VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16;
    case Pengine::Texture::Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
        return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16;
    case Pengine::Texture::Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
        return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16;
    case Pengine::Texture::Format::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
        return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16;
    case Pengine::Texture::Format::G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
        return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16;
    case Pengine::Texture::Format::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
        return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16;
    case Pengine::Texture::Format::G16B16G16R16_422_UNORM:
        return VK_FORMAT_G16B16G16R16_422_UNORM;
    case Pengine::Texture::Format::B16G16R16G16_422_UNORM:
        return VK_FORMAT_B16G16R16G16_422_UNORM;
    case Pengine::Texture::Format::G16_B16_R16_3PLANE_420_UNORM:
        return VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM;
    case Pengine::Texture::Format::G16_B16R16_2PLANE_420_UNORM:
        return VK_FORMAT_G16_B16R16_2PLANE_420_UNORM;
    case Pengine::Texture::Format::G16_B16_R16_3PLANE_422_UNORM:
        return VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM;
    case Pengine::Texture::Format::G16_B16R16_2PLANE_422_UNORM:
        return VK_FORMAT_G16_B16R16_2PLANE_422_UNORM;
    case Pengine::Texture::Format::G16_B16_R16_3PLANE_444_UNORM:
        return VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM;
    case Pengine::Texture::Format::G8_B8R8_2PLANE_444_UNORM:
        return VK_FORMAT_G8_B8R8_2PLANE_444_UNORM;
    case Pengine::Texture::Format::G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
        return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16;
    case Pengine::Texture::Format::G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
        return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16;
    case Pengine::Texture::Format::G16_B16R16_2PLANE_444_UNORM:
        return VK_FORMAT_G16_B16R16_2PLANE_444_UNORM;
    case Pengine::Texture::Format::A4R4G4B4_UNORM_PACK16:
        return VK_FORMAT_A4R4G4B4_UNORM_PACK16;
    case Pengine::Texture::Format::A4B4G4R4_UNORM_PACK16:
        return VK_FORMAT_A4B4G4R4_UNORM_PACK16;
    case Pengine::Texture::Format::ASTC_4x4_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK;
    case Pengine::Texture::Format::ASTC_5x4_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK;
    case Pengine::Texture::Format::ASTC_5x5_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK;
    case Pengine::Texture::Format::ASTC_6x5_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK;
    case Pengine::Texture::Format::ASTC_6x6_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK;
    case Pengine::Texture::Format::ASTC_8x5_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK;
    case Pengine::Texture::Format::ASTC_8x6_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK;
    case Pengine::Texture::Format::ASTC_8x8_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK;
    case Pengine::Texture::Format::ASTC_10x5_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK;
    case Pengine::Texture::Format::ASTC_10x6_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK;
    case Pengine::Texture::Format::ASTC_10x8_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK;
    case Pengine::Texture::Format::ASTC_10x10_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK;
    case Pengine::Texture::Format::ASTC_12x10_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK;
    case Pengine::Texture::Format::ASTC_12x12_SFLOAT_BLOCK:
        return VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK;
    case Pengine::Texture::Format::PVRTC1_2BPP_UNORM_BLOCK_IMG:
        return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
    case Pengine::Texture::Format::PVRTC1_4BPP_UNORM_BLOCK_IMG:
        return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
    case Pengine::Texture::Format::PVRTC2_2BPP_UNORM_BLOCK_IMG:
        return VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG;
    case Pengine::Texture::Format::PVRTC2_4BPP_UNORM_BLOCK_IMG:
        return VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG;
    case Pengine::Texture::Format::PVRTC1_2BPP_SRGB_BLOCK_IMG:
        return VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG;
    case Pengine::Texture::Format::PVRTC1_4BPP_SRGB_BLOCK_IMG:
        return VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG;
    case Pengine::Texture::Format::PVRTC2_2BPP_SRGB_BLOCK_IMG:
        return VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG;
    case Pengine::Texture::Format::PVRTC2_4BPP_SRGB_BLOCK_IMG:
        return VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG;
    case Pengine::Texture::Format::R16G16_S10_5_NV:
        return VK_FORMAT_R16G16_S10_5_NV;
    case Pengine::Texture::Format::MAX_ENUM:
        return VK_FORMAT_MAX_ENUM;
    default:
        return VK_FORMAT_UNDEFINED;
    }

    FATAL_ERROR("Failed to convert format!")
}

Texture::Format VulkanTexture::ConvertFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_UNDEFINED:
        return Pengine::Texture::Format::UNDEFINED;
    case VK_FORMAT_R4G4_UNORM_PACK8:
        return Pengine::Texture::Format::R4G4_UNORM_PACK8;
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        return Pengine::Texture::Format::R4G4B4A4_UNORM_PACK16;
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        return Pengine::Texture::Format::B4G4R4A4_UNORM_PACK16;
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
        return Pengine::Texture::Format::R5G6B5_UNORM_PACK16;
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
        return Pengine::Texture::Format::B5G6R5_UNORM_PACK16;
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        return Pengine::Texture::Format::R5G5B5A1_UNORM_PACK16;
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        return Pengine::Texture::Format::B5G5R5A1_UNORM_PACK16;
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        return Pengine::Texture::Format::A1R5G5B5_UNORM_PACK16;
    case VK_FORMAT_R8_UNORM:
        return Pengine::Texture::Format::R8_UNORM;
    case VK_FORMAT_R8_SNORM:
        return Pengine::Texture::Format::R8_SNORM;
    case VK_FORMAT_R8_USCALED:
        return Pengine::Texture::Format::R8_USCALED;
    case VK_FORMAT_R8_SSCALED:
        return Pengine::Texture::Format::R8_SSCALED;
    case VK_FORMAT_R8_UINT:
        return Pengine::Texture::Format::R8_UINT;
    case VK_FORMAT_R8_SINT:
        return Pengine::Texture::Format::R8_SINT;
    case VK_FORMAT_R8_SRGB:
        return Pengine::Texture::Format::R8_SRGB;
    case VK_FORMAT_R8G8_UNORM:
        return Pengine::Texture::Format::R8G8_UNORM;
    case VK_FORMAT_R8G8_SNORM:
        return Pengine::Texture::Format::R8G8_SNORM;
    case VK_FORMAT_R8G8_USCALED:
        return Pengine::Texture::Format::R8G8_USCALED;
    case VK_FORMAT_R8G8_SSCALED:
        return Pengine::Texture::Format::R8G8_SSCALED;
    case VK_FORMAT_R8G8_UINT:
        return Pengine::Texture::Format::R8G8_UINT;
    case VK_FORMAT_R8G8_SINT:
        return Pengine::Texture::Format::R8G8_SINT;
    case VK_FORMAT_R8G8_SRGB:
        return Pengine::Texture::Format::R8G8_SRGB;
    case VK_FORMAT_R8G8B8_UNORM:
        return Pengine::Texture::Format::R8G8B8_UNORM;
    case VK_FORMAT_R8G8B8_SNORM:
        return Pengine::Texture::Format::R8G8B8_SNORM;
    case VK_FORMAT_R8G8B8_USCALED:
        return Pengine::Texture::Format::R8G8B8_USCALED;
    case VK_FORMAT_R8G8B8_SSCALED:
        return Pengine::Texture::Format::R8G8B8_SSCALED;
    case VK_FORMAT_R8G8B8_UINT:
        return Pengine::Texture::Format::R8G8B8_UINT;
    case VK_FORMAT_R8G8B8_SINT:
        return Pengine::Texture::Format::R8G8B8_SINT;
    case VK_FORMAT_R8G8B8_SRGB:
        return Pengine::Texture::Format::R8G8B8_SRGB;
    case VK_FORMAT_B8G8R8_UNORM:
        return Pengine::Texture::Format::B8G8R8_UNORM;
    case VK_FORMAT_B8G8R8_SNORM:
        return Pengine::Texture::Format::B8G8R8_SNORM;
    case VK_FORMAT_B8G8R8_USCALED:
        return Pengine::Texture::Format::B8G8R8_USCALED;
    case VK_FORMAT_B8G8R8_SSCALED:
        return Pengine::Texture::Format::B8G8R8_SSCALED;
    case VK_FORMAT_B8G8R8_UINT:
        return Pengine::Texture::Format::B8G8R8_UINT;
    case VK_FORMAT_B8G8R8_SINT:
        return Pengine::Texture::Format::B8G8R8_SINT;
    case VK_FORMAT_B8G8R8_SRGB:
        return Pengine::Texture::Format::B8G8R8_SRGB;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return Pengine::Texture::Format::R8G8B8A8_UNORM;
    case VK_FORMAT_R8G8B8A8_SNORM:
        return Pengine::Texture::Format::R8G8B8A8_SNORM;
    case VK_FORMAT_R8G8B8A8_USCALED:
        return Pengine::Texture::Format::R8G8B8A8_USCALED;
    case VK_FORMAT_R8G8B8A8_SSCALED:
        return Pengine::Texture::Format::R8G8B8A8_SSCALED;
    case VK_FORMAT_R8G8B8A8_UINT:
        return Pengine::Texture::Format::R8G8B8A8_UINT;
    case VK_FORMAT_R8G8B8A8_SINT:
        return Pengine::Texture::Format::R8G8B8A8_SINT;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return Pengine::Texture::Format::R8G8B8A8_SRGB;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return Pengine::Texture::Format::B8G8R8A8_UNORM;
    case VK_FORMAT_B8G8R8A8_SNORM:
        return Pengine::Texture::Format::B8G8R8A8_SNORM;
    case VK_FORMAT_B8G8R8A8_USCALED:
        return Pengine::Texture::Format::B8G8R8A8_USCALED;
    case VK_FORMAT_B8G8R8A8_SSCALED:
        return Pengine::Texture::Format::B8G8R8A8_SSCALED;
    case VK_FORMAT_B8G8R8A8_UINT:
        return Pengine::Texture::Format::B8G8R8A8_UINT;
    case VK_FORMAT_B8G8R8A8_SINT:
        return Pengine::Texture::Format::B8G8R8A8_SINT;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return Pengine::Texture::Format::B8G8R8A8_SRGB;
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        return Pengine::Texture::Format::A8B8G8R8_UNORM_PACK32;
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        return Pengine::Texture::Format::A8B8G8R8_SNORM_PACK32;
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        return Pengine::Texture::Format::A8B8G8R8_USCALED_PACK32;
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        return Pengine::Texture::Format::A8B8G8R8_SSCALED_PACK32;
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        return Pengine::Texture::Format::A8B8G8R8_UINT_PACK32;
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        return Pengine::Texture::Format::A8B8G8R8_SINT_PACK32;
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        return Pengine::Texture::Format::A8B8G8R8_SRGB_PACK32;
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        return Pengine::Texture::Format::A2R10G10B10_UNORM_PACK32;
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        return Pengine::Texture::Format::A2R10G10B10_SNORM_PACK32;
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        return Pengine::Texture::Format::A2R10G10B10_USCALED_PACK32;
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        return Pengine::Texture::Format::A2R10G10B10_SSCALED_PACK32;
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        return Pengine::Texture::Format::A2R10G10B10_UINT_PACK32;
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        return Pengine::Texture::Format::A2R10G10B10_SINT_PACK32;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        return Pengine::Texture::Format::A2B10G10R10_UNORM_PACK32;
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        return Pengine::Texture::Format::A2B10G10R10_SNORM_PACK32;
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        return Pengine::Texture::Format::A2B10G10R10_USCALED_PACK32;
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        return Pengine::Texture::Format::A2B10G10R10_SSCALED_PACK32;
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        return Pengine::Texture::Format::A2B10G10R10_UINT_PACK32;
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        return Pengine::Texture::Format::A2B10G10R10_SINT_PACK32;
    case VK_FORMAT_R16_UNORM:
        return Pengine::Texture::Format::R16_UNORM;
    case VK_FORMAT_R16_SNORM:
        return Pengine::Texture::Format::R16_SNORM;
    case VK_FORMAT_R16_USCALED:
        return Pengine::Texture::Format::R16_USCALED;
    case VK_FORMAT_R16_SSCALED:
        return Pengine::Texture::Format::R16_SSCALED;
    case VK_FORMAT_R16_UINT:
        return Pengine::Texture::Format::R16_UINT;
    case VK_FORMAT_R16_SINT:
        return Pengine::Texture::Format::R16_SINT;
    case VK_FORMAT_R16_SFLOAT:
        return Pengine::Texture::Format::R16_SFLOAT;
    case VK_FORMAT_R16G16_UNORM:
        return Pengine::Texture::Format::R16G16_UNORM;
    case VK_FORMAT_R16G16_SNORM:
        return Pengine::Texture::Format::R16G16_SNORM;
    case VK_FORMAT_R16G16_USCALED:
        return Pengine::Texture::Format::R16G16_USCALED;
    case VK_FORMAT_R16G16_SSCALED:
        return Pengine::Texture::Format::R16G16_SSCALED;
    case VK_FORMAT_R16G16_UINT:
        return Pengine::Texture::Format::R16G16_UINT;
    case VK_FORMAT_R16G16_SINT:
        return Pengine::Texture::Format::R16G16_SINT;
    case VK_FORMAT_R16G16_SFLOAT:
        return Pengine::Texture::Format::R16G16_SFLOAT;
    case VK_FORMAT_R16G16B16_UNORM:
        return Pengine::Texture::Format::R16G16B16_UNORM;
    case VK_FORMAT_R16G16B16_SNORM:
        return Pengine::Texture::Format::R16G16B16_SNORM;
    case VK_FORMAT_R16G16B16_USCALED:
        return Pengine::Texture::Format::R16G16B16_USCALED;
    case VK_FORMAT_R16G16B16_SSCALED:
        return Pengine::Texture::Format::R16G16B16_SSCALED;
    case VK_FORMAT_R16G16B16_UINT:
        return Pengine::Texture::Format::R16G16B16_UINT;
    case VK_FORMAT_R16G16B16_SINT:
        return Pengine::Texture::Format::R16G16B16_SINT;
    case VK_FORMAT_R16G16B16_SFLOAT:
        return Pengine::Texture::Format::R16G16B16_SFLOAT;
    case VK_FORMAT_R16G16B16A16_UNORM:
        return Pengine::Texture::Format::R16G16B16A16_UNORM;
    case VK_FORMAT_R16G16B16A16_SNORM:
        return Pengine::Texture::Format::R16G16B16A16_SNORM;
    case VK_FORMAT_R16G16B16A16_USCALED:
        return Pengine::Texture::Format::R16G16B16A16_USCALED;
    case VK_FORMAT_R16G16B16A16_SSCALED:
        return Pengine::Texture::Format::R16G16B16A16_SSCALED;
    case VK_FORMAT_R16G16B16A16_UINT:
        return Pengine::Texture::Format::R16G16B16A16_UINT;
    case VK_FORMAT_R16G16B16A16_SINT:
        return Pengine::Texture::Format::R16G16B16A16_SINT;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return Pengine::Texture::Format::R16G16B16A16_SFLOAT;
    case VK_FORMAT_R32_UINT:
        return Pengine::Texture::Format::R32_UINT;
    case VK_FORMAT_R32_SINT:
        return Pengine::Texture::Format::R32_SINT;
    case VK_FORMAT_R32_SFLOAT:
        return Pengine::Texture::Format::R32_SFLOAT;
    case VK_FORMAT_R32G32_UINT:
        return Pengine::Texture::Format::R32G32_UINT;
    case VK_FORMAT_R32G32_SINT:
        return Pengine::Texture::Format::R32G32_SINT;
    case VK_FORMAT_R32G32_SFLOAT:
        return Pengine::Texture::Format::R32G32_SFLOAT;
    case VK_FORMAT_R32G32B32_UINT:
        return Pengine::Texture::Format::R32G32B32_UINT;
    case VK_FORMAT_R32G32B32_SINT:
        return Pengine::Texture::Format::R32G32B32_SINT;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return Pengine::Texture::Format::R32G32B32_SFLOAT;
    case VK_FORMAT_R32G32B32A32_UINT:
        return Pengine::Texture::Format::R32G32B32A32_UINT;
    case VK_FORMAT_R32G32B32A32_SINT:
        return Pengine::Texture::Format::R32G32B32A32_SINT;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return Pengine::Texture::Format::R32G32B32A32_SFLOAT;
    case VK_FORMAT_R64_UINT:
        return Pengine::Texture::Format::R64_UINT;
    case VK_FORMAT_R64_SINT:
        return Pengine::Texture::Format::R64_SINT;
    case VK_FORMAT_R64_SFLOAT:
        return Pengine::Texture::Format::R64_SFLOAT;
    case VK_FORMAT_R64G64_UINT:
        return Pengine::Texture::Format::R64G64_UINT;
    case VK_FORMAT_R64G64_SINT:
        return Pengine::Texture::Format::R64G64_SINT;
    case VK_FORMAT_R64G64_SFLOAT:
        return Pengine::Texture::Format::R64G64_SFLOAT;
    case VK_FORMAT_R64G64B64_UINT:
        return Pengine::Texture::Format::R64G64B64_UINT;
    case VK_FORMAT_R64G64B64_SINT:
        return Pengine::Texture::Format::R64G64B64_SINT;
    case VK_FORMAT_R64G64B64_SFLOAT:
        return Pengine::Texture::Format::R64G64B64_SFLOAT;
    case VK_FORMAT_R64G64B64A64_UINT:
        return Pengine::Texture::Format::R64G64B64A64_UINT;
    case VK_FORMAT_R64G64B64A64_SINT:
        return Pengine::Texture::Format::R64G64B64A64_SINT;
    case VK_FORMAT_R64G64B64A64_SFLOAT:
        return Pengine::Texture::Format::R64G64B64A64_SFLOAT;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        return Pengine::Texture::Format::B10G11R11_UFLOAT_PACK32;
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        return Pengine::Texture::Format::E5B9G9R9_UFLOAT_PACK32;
    case VK_FORMAT_D16_UNORM:
        return Pengine::Texture::Format::D16_UNORM;
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return Pengine::Texture::Format::X8_D24_UNORM_PACK32;
    case VK_FORMAT_D32_SFLOAT:
        return Pengine::Texture::Format::D32_SFLOAT;
    case VK_FORMAT_S8_UINT:
        return Pengine::Texture::Format::S8_UINT;
    case VK_FORMAT_D16_UNORM_S8_UINT:
        return Pengine::Texture::Format::D16_UNORM_S8_UINT;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return Pengine::Texture::Format::D24_UNORM_S8_UINT;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return Pengine::Texture::Format::D32_SFLOAT_S8_UINT;
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        return Pengine::Texture::Format::BC1_RGB_UNORM_BLOCK;
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        return Pengine::Texture::Format::BC1_RGB_SRGB_BLOCK;
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        return Pengine::Texture::Format::BC1_RGBA_UNORM_BLOCK;
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        return Pengine::Texture::Format::BC1_RGBA_SRGB_BLOCK;
    case VK_FORMAT_BC2_UNORM_BLOCK:
        return Pengine::Texture::Format::BC2_UNORM_BLOCK;
    case VK_FORMAT_BC2_SRGB_BLOCK:
        return Pengine::Texture::Format::BC2_SRGB_BLOCK;
    case VK_FORMAT_BC3_UNORM_BLOCK:
        return Pengine::Texture::Format::BC3_UNORM_BLOCK;
    case VK_FORMAT_BC3_SRGB_BLOCK:
        return Pengine::Texture::Format::BC3_SRGB_BLOCK;
    case VK_FORMAT_BC4_UNORM_BLOCK:
        return Pengine::Texture::Format::BC4_UNORM_BLOCK;
    case VK_FORMAT_BC4_SNORM_BLOCK:
        return Pengine::Texture::Format::BC4_SNORM_BLOCK;
    case VK_FORMAT_BC5_UNORM_BLOCK:
        return Pengine::Texture::Format::BC5_UNORM_BLOCK;
    case VK_FORMAT_BC5_SNORM_BLOCK:
        return Pengine::Texture::Format::BC5_SNORM_BLOCK;
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        return Pengine::Texture::Format::BC6H_UFLOAT_BLOCK;
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        return Pengine::Texture::Format::BC6H_SFLOAT_BLOCK;
    case VK_FORMAT_BC7_UNORM_BLOCK:
        return Pengine::Texture::Format::BC7_UNORM_BLOCK;
    case VK_FORMAT_BC7_SRGB_BLOCK:
        return Pengine::Texture::Format::BC7_SRGB_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        return Pengine::Texture::Format::ETC2_R8G8B8_UNORM_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
        return Pengine::Texture::Format::ETC2_R8G8B8_SRGB_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        return Pengine::Texture::Format::ETC2_R8G8B8A1_UNORM_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
        return Pengine::Texture::Format::ETC2_R8G8B8A1_SRGB_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        return Pengine::Texture::Format::ETC2_R8G8B8A8_UNORM_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
        return Pengine::Texture::Format::ETC2_R8G8B8A8_SRGB_BLOCK;
    case VK_FORMAT_EAC_R11_UNORM_BLOCK:
        return Pengine::Texture::Format::EAC_R11_UNORM_BLOCK;
    case VK_FORMAT_EAC_R11_SNORM_BLOCK:
        return Pengine::Texture::Format::EAC_R11_SNORM_BLOCK;
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
        return Pengine::Texture::Format::EAC_R11G11_UNORM_BLOCK;
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
        return Pengine::Texture::Format::EAC_R11G11_SNORM_BLOCK;
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_4x4_UNORM_BLOCK;
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_4x4_SRGB_BLOCK;
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_5x4_UNORM_BLOCK;
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_5x4_SRGB_BLOCK;
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_5x5_UNORM_BLOCK;
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_5x5_SRGB_BLOCK;
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_6x5_UNORM_BLOCK;
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_6x5_SRGB_BLOCK;
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_6x6_UNORM_BLOCK;
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_6x6_SRGB_BLOCK;
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_8x5_UNORM_BLOCK;
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_8x5_SRGB_BLOCK;
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_8x6_UNORM_BLOCK;
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_8x6_SRGB_BLOCK;
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_8x8_UNORM_BLOCK;
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_8x8_SRGB_BLOCK;
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_10x5_UNORM_BLOCK;
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_10x5_SRGB_BLOCK;
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_10x6_UNORM_BLOCK;
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_10x6_SRGB_BLOCK;
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_10x8_UNORM_BLOCK;
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_10x8_SRGB_BLOCK;
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_10x10_UNORM_BLOCK;
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_10x10_SRGB_BLOCK;
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_12x10_UNORM_BLOCK;
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_12x10_SRGB_BLOCK;
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
        return Pengine::Texture::Format::ASTC_12x12_UNORM_BLOCK;
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
        return Pengine::Texture::Format::ASTC_12x12_SRGB_BLOCK;
    case VK_FORMAT_G8B8G8R8_422_UNORM:
        return Pengine::Texture::Format::G8B8G8R8_422_UNORM;
    case VK_FORMAT_B8G8R8G8_422_UNORM:
        return Pengine::Texture::Format::B8G8R8G8_422_UNORM;
    case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
        return Pengine::Texture::Format::G8_B8_R8_3PLANE_420_UNORM;
    case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
        return Pengine::Texture::Format::G8_B8R8_2PLANE_420_UNORM;
    case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
        return Pengine::Texture::Format::G8_B8_R8_3PLANE_422_UNORM;
    case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
        return Pengine::Texture::Format::G8_B8R8_2PLANE_422_UNORM;
    case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
        return Pengine::Texture::Format::G8_B8_R8_3PLANE_444_UNORM;
    case VK_FORMAT_R10X6_UNORM_PACK16:
        return Pengine::Texture::Format::R10X6_UNORM_PACK16;
    case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
        return Pengine::Texture::Format::R10X6G10X6_UNORM_2PACK16;
    case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
        return Pengine::Texture::Format::R10X6G10X6B10X6A10X6_UNORM_4PACK16;
    case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
        return Pengine::Texture::Format::G10X6B10X6G10X6R10X6_422_UNORM_4PACK16;
    case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
        return Pengine::Texture::Format::B10X6G10X6R10X6G10X6_422_UNORM_4PACK16;
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
        return Pengine::Texture::Format::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16;
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        return Pengine::Texture::Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
        return Pengine::Texture::Format::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16;
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        return Pengine::Texture::Format::G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16;
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        return Pengine::Texture::Format::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16;
    case VK_FORMAT_R12X4_UNORM_PACK16:
        return Pengine::Texture::Format::R12X4_UNORM_PACK16;
    case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
        return Pengine::Texture::Format::R12X4G12X4_UNORM_2PACK16;
    case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
        return Pengine::Texture::Format::R12X4G12X4B12X4A12X4_UNORM_4PACK16;
    case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
        return Pengine::Texture::Format::G12X4B12X4G12X4R12X4_422_UNORM_4PACK16;
    case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
        return Pengine::Texture::Format::B12X4G12X4R12X4G12X4_422_UNORM_4PACK16;
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
        return Pengine::Texture::Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16;
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
        return Pengine::Texture::Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16;
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
        return Pengine::Texture::Format::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16;
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
        return Pengine::Texture::Format::G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16;
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
        return Pengine::Texture::Format::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16;
    case VK_FORMAT_G16B16G16R16_422_UNORM:
        return Pengine::Texture::Format::G16B16G16R16_422_UNORM;
    case VK_FORMAT_B16G16R16G16_422_UNORM:
        return Pengine::Texture::Format::B16G16R16G16_422_UNORM;
    case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
        return Pengine::Texture::Format::G16_B16_R16_3PLANE_420_UNORM;
    case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
        return Pengine::Texture::Format::G16_B16R16_2PLANE_420_UNORM;
    case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
        return Pengine::Texture::Format::G16_B16_R16_3PLANE_422_UNORM;
    case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
        return Pengine::Texture::Format::G16_B16R16_2PLANE_422_UNORM;
    case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
        return Pengine::Texture::Format::G16_B16_R16_3PLANE_444_UNORM;
    case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:
        return Pengine::Texture::Format::G8_B8R8_2PLANE_444_UNORM;
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
        return Pengine::Texture::Format::G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16;
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
        return Pengine::Texture::Format::G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16;
    case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:
        return Pengine::Texture::Format::G16_B16R16_2PLANE_444_UNORM;
    case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
        return Pengine::Texture::Format::A4R4G4B4_UNORM_PACK16;
    case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
        return Pengine::Texture::Format::A4B4G4R4_UNORM_PACK16;
    case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_4x4_SFLOAT_BLOCK;
    case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_5x4_SFLOAT_BLOCK;
    case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_5x5_SFLOAT_BLOCK;
    case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_6x5_SFLOAT_BLOCK;
    case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_6x6_SFLOAT_BLOCK;
    case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_8x5_SFLOAT_BLOCK;
    case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_8x6_SFLOAT_BLOCK;
    case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_8x8_SFLOAT_BLOCK;
    case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_10x5_SFLOAT_BLOCK;
    case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_10x6_SFLOAT_BLOCK;
    case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_10x8_SFLOAT_BLOCK;
    case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_10x10_SFLOAT_BLOCK;
    case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_12x10_SFLOAT_BLOCK;
    case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
        return Pengine::Texture::Format::ASTC_12x12_SFLOAT_BLOCK;
    case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
        return Pengine::Texture::Format::PVRTC1_2BPP_UNORM_BLOCK_IMG;
    case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
        return Pengine::Texture::Format::PVRTC1_4BPP_UNORM_BLOCK_IMG;
    case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
        return Pengine::Texture::Format::PVRTC2_2BPP_UNORM_BLOCK_IMG;
    case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
        return Pengine::Texture::Format::PVRTC2_4BPP_UNORM_BLOCK_IMG;
    case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
        return Pengine::Texture::Format::PVRTC1_2BPP_SRGB_BLOCK_IMG;
    case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
        return Pengine::Texture::Format::PVRTC1_4BPP_SRGB_BLOCK_IMG;
    case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
        return Pengine::Texture::Format::PVRTC2_2BPP_SRGB_BLOCK_IMG;
    case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
        return Pengine::Texture::Format::PVRTC2_4BPP_SRGB_BLOCK_IMG;
    case VK_FORMAT_R16G16_S10_5_NV:
        return Pengine::Texture::Format::R16G16_S10_5_NV;
    case VK_FORMAT_MAX_ENUM:
        return Pengine::Texture::Format::MAX_ENUM;
    default:
        return Pengine::Texture::Format::UNDEFINED;
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
