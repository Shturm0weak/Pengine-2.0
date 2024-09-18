#include "VulkanTexture.h"

#include "VulkanBuffer.h"
#include "VulkanDescriptors.h"
#include "VulkanDevice.h"
#include "VulkanFormat.h"
#include "VulkanSamplerManager.h"

#include "../Core/Logger.h"

#include <imgui/backends/imgui_impl_vulkan.h>

using namespace Pengine;
using namespace Vk;

VulkanTexture::VulkanTexture(const CreateInfo& createInfo)
	: Texture(createInfo)
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
	imageInfo.arrayLayers = m_LayerCount;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = createInfo.isCubeMap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

	for (Usage usage : createInfo.usage)
	{
		imageInfo.usage |= ConvertUsage(usage);
	}

	device->CreateImage(
		imageInfo,
		m_Image,
		m_VmaAllocation,
		m_VmaAllocationInfo);

	if (createInfo.data)
	{
		std::shared_ptr<VulkanBuffer> stagingBuffer = VulkanBuffer::CreateStagingBuffer(
			createInfo.channels * createInfo.instanceSize,
			m_Size.x * m_Size.y);

		stagingBuffer->WriteToBuffer(createInfo.data, stagingBuffer->GetSize());

		TransitionToWrite();

		device->CopyBufferToImage(
			stagingBuffer->GetBuffer(),
			m_Image,
			static_cast<uint32_t>(m_Size.x),
			static_cast<uint32_t>(m_Size.y));

		if (m_MipLevels > 1)
		{
			GenerateMipMaps();
		}
		else
		{
			TransitionToRead();
		}
	}

	VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
	if (createInfo.isCubeMap)
	{
		imageViewType = VK_IMAGE_VIEW_TYPE_CUBE;
	}
	else if (createInfo.layerCount > 1)
	{
		imageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}

	m_View = CreateImageView(
		m_Image,
		format,
		aspectMask,
		m_MipLevels,
		m_LayerCount,
		imageViewType);
	m_Sampler = CreateSampler(createInfo.samplerCreateInfo);

	std::unique_ptr<VulkanDescriptorSetLayout> setLayout = VulkanDescriptorSetLayout::Builder()
		.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.Build();

	VkDescriptorImageInfo descriptorImageInfo = GetDescriptorInfo();
	VulkanDescriptorWriter(*setLayout, *descriptorPool)
		.WriteImage(0, &descriptorImageInfo)
		.Build(m_DescriptorSet);
}

VulkanTexture::~VulkanTexture()
{
	vkDeviceWaitIdle(device->GetDevice());

	vkDestroyImageView(device->GetDevice(), m_View, nullptr);
	vmaDestroyImage(device->GetVmaAllocator(), m_Image, m_VmaAllocation);

	std::vector<VkDescriptorSet> descriptorSets = { m_DescriptorSet };
	descriptorPool->FreeDescriptors(descriptorSets);
}

VkDescriptorImageInfo VulkanTexture::GetDescriptorInfo()
{
	VkDescriptorImageInfo descriptorImageInfo{};
	descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	descriptorImageInfo.imageView = m_View;
	descriptorImageInfo.sampler = m_Sampler;
	
	return descriptorImageInfo;
}

VkImageView VulkanTexture::CreateImageView(
	const VkImage image,
	const VkFormat format,
	const VkImageAspectFlagBits aspectMask,
	const uint32_t mipLevels,
	uint32_t layerCount,
	VkImageViewType imageViewType)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = imageViewType;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectMask;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = layerCount;

	VkImageView imageView;
	if (vkCreateImageView(device->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to create texture image view!");
	}

	return imageView;
}

VkSampler VulkanTexture::CreateSampler(const Texture::SamplerCreateInfo& samplerCreateInfo)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = ConvertFilter(samplerCreateInfo.filter);
	samplerInfo.minFilter = ConvertFilter(samplerCreateInfo.filter);
	samplerInfo.addressModeU = ConvertAddressMode(samplerCreateInfo.addressMode);
	samplerInfo.addressModeV = ConvertAddressMode(samplerCreateInfo.addressMode);
	samplerInfo.addressModeW = ConvertAddressMode(samplerCreateInfo.addressMode);
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = samplerCreateInfo.maxAnisotropy;
	samplerInfo.borderColor = ConvertBorderColor(samplerCreateInfo.borderColor);
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = ConvertMipmapMode(samplerCreateInfo.mipmapMode);
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

	return VulkanSamplerManager::GetInstance().CreateSampler(samplerInfo);
}

VkImageAspectFlagBits VulkanTexture::ConvertAspectMask(const AspectMask aspectMask)
{
	switch (aspectMask)
	{
	case Pengine::Texture::AspectMask::COLOR:
		return VK_IMAGE_ASPECT_COLOR_BIT;
	case Pengine::Texture::AspectMask::DEPTH:
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	FATAL_ERROR("Failed to convert aspect mask!");
	return VK_IMAGE_ASPECT_NONE;
}

Texture::AspectMask VulkanTexture::ConvertAspectMask(const VkImageAspectFlagBits aspectMask)
{
	switch (aspectMask)
	{
	case VK_IMAGE_ASPECT_COLOR_BIT:
		return Pengine::Texture::AspectMask::COLOR;
	case VK_IMAGE_ASPECT_DEPTH_BIT:
		return Pengine::Texture::AspectMask::DEPTH;
	}

	FATAL_ERROR("Failed to convert aspect mask!");
	return {};
}

VkImageLayout VulkanTexture::ConvertLayout(const Layout layout)
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

	FATAL_ERROR("Failed to convert texture layout!");
	return VK_IMAGE_LAYOUT_UNDEFINED;
}

Texture::Layout VulkanTexture::ConvertLayout(const VkImageLayout layout)
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

	FATAL_ERROR("Failed to convert texture layout!");
	return Texture::Layout::UNDEFINED;
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

	FATAL_ERROR("Failed to convert usage!");
	return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
}

Texture::Usage VulkanTexture::ConvertUsage(const VkImageUsageFlagBits usage)
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

	FATAL_ERROR("Failed to convert usage!");
	return {};
}

VkSamplerAddressMode VulkanTexture::ConvertAddressMode(SamplerCreateInfo::AddressMode addressMode)
{
	switch (addressMode)
	{
	case Pengine::Texture::SamplerCreateInfo::AddressMode::REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case Pengine::Texture::SamplerCreateInfo::AddressMode::MIRRORED_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case Pengine::Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case Pengine::Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case Pengine::Texture::SamplerCreateInfo::AddressMode::MIRROR_CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	default:
		break;
	}

	FATAL_ERROR("Failed to convert address mode!");
	return {};
}

Texture::SamplerCreateInfo::AddressMode VulkanTexture::ConvertAddressMode(VkSamplerAddressMode addressMode)
{
	switch (addressMode)
	{
	case VK_SAMPLER_ADDRESS_MODE_REPEAT:
		return Pengine::Texture::SamplerCreateInfo::AddressMode::REPEAT;
	case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
		return Pengine::Texture::SamplerCreateInfo::AddressMode::MIRRORED_REPEAT;
	case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
		return Pengine::Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_EDGE;
	case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
		return Pengine::Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER;
	case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
		return Pengine::Texture::SamplerCreateInfo::AddressMode::MIRROR_CLAMP_TO_EDGE;
	default:
		break;
	}

	FATAL_ERROR("Failed to convert address mode!");
	return {};
}

VkFilter VulkanTexture::ConvertFilter(SamplerCreateInfo::Filter filter)
{
	switch (filter)
	{
	case Pengine::Texture::SamplerCreateInfo::Filter::NEAREST:
		return VK_FILTER_NEAREST;
	case Pengine::Texture::SamplerCreateInfo::Filter::LINEAR:
		return VK_FILTER_LINEAR;
	default:
		break;
	}

	FATAL_ERROR("Failed to convert filter!");
	return {};
}

Texture::SamplerCreateInfo::Filter VulkanTexture::ConvertFilter(VkFilter filter)
{
	switch (filter)
	{
	case VK_FILTER_NEAREST:
		return Pengine::Texture::SamplerCreateInfo::Filter::NEAREST;
	case VK_FILTER_LINEAR:
		return Pengine::Texture::SamplerCreateInfo::Filter::LINEAR;
	default:
		break;
	}

	FATAL_ERROR("Failed to convert filter!");
	return {};
}

VkSamplerMipmapMode VulkanTexture::ConvertMipmapMode(Texture::SamplerCreateInfo::MipmapMode mipmapMode)
{
	switch (mipmapMode)
	{
	case Pengine::Texture::SamplerCreateInfo::MipmapMode::MODE_NEAREST:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case Pengine::Texture::SamplerCreateInfo::MipmapMode::MODE_LINEAR:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	default:
		break;
	}

	FATAL_ERROR("Failed to convert mipmap mode!");
	return {};
}

Texture::SamplerCreateInfo::MipmapMode VulkanTexture::ConvertMipmapMode(VkSamplerMipmapMode mipmapMode)
{
	switch (mipmapMode)
	{
	case VK_SAMPLER_MIPMAP_MODE_NEAREST:
		return Pengine::Texture::SamplerCreateInfo::MipmapMode::MODE_NEAREST;
	case VK_SAMPLER_MIPMAP_MODE_LINEAR:
		return Pengine::Texture::SamplerCreateInfo::MipmapMode::MODE_LINEAR;
	default:
		break;
	}

	FATAL_ERROR("Failed to convert mipmap mode!");
	return {};
}

VkBorderColor VulkanTexture::ConvertBorderColor(Texture::SamplerCreateInfo::BorderColor borderColor)
{
	switch (borderColor)
	{
	case Pengine::Texture::SamplerCreateInfo::BorderColor::FLOAT_TRANSPARENT_BLACK:
		return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	case Pengine::Texture::SamplerCreateInfo::BorderColor::INT_TRANSPARENT_BLACK:
		return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	case Pengine::Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_BLACK:
		return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	case Pengine::Texture::SamplerCreateInfo::BorderColor::INT_OPAQUE_BLACK:
		return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	case Pengine::Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_WHITE:
		return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	case Pengine::Texture::SamplerCreateInfo::BorderColor::INT_OPAQUE_WHITE:
		return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	default:
		break;
	}

	FATAL_ERROR("Failed to convert border color!");
	return {};
}

Texture::SamplerCreateInfo::BorderColor VulkanTexture::ConvertBorderColor(VkBorderColor borderColor)
{
	switch (borderColor)
	{
	case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
		return Pengine::Texture::SamplerCreateInfo::BorderColor::FLOAT_TRANSPARENT_BLACK;
	case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:
		return Pengine::Texture::SamplerCreateInfo::BorderColor::INT_TRANSPARENT_BLACK;
	case VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
		return Pengine::Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_BLACK;
	case VK_BORDER_COLOR_INT_OPAQUE_BLACK:
		return Pengine::Texture::SamplerCreateInfo::BorderColor::INT_OPAQUE_BLACK;
	case VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
		return Pengine::Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_WHITE;
	case VK_BORDER_COLOR_INT_OPAQUE_WHITE:
		return Pengine::Texture::SamplerCreateInfo::BorderColor::INT_OPAQUE_WHITE;
	default:
		break;
	}

	FATAL_ERROR("Failed to convert border color!");
	return {};
}

void VulkanTexture::GenerateMipMaps()
{
	Vk::device->GenerateMipMaps(m_Image, ConvertFormat(m_Format), m_Size.x, m_Size.y, m_MipLevels, m_LayerCount);
}

void VulkanTexture::TransitionToWrite()
{
	device->TransitionImageLayout(
		m_Image,
		ConvertFormat(m_Format),
		ConvertAspectMask(m_AspectMask),
		m_Layout,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		m_MipLevels,
		m_LayerCount);

	m_Layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
}

void VulkanTexture::TransitionToRead()
{
	device->TransitionImageLayout(
		m_Image,
		ConvertFormat(m_Format),
		ConvertAspectMask(m_AspectMask),
		m_Layout,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		m_MipLevels,
		m_LayerCount);

	m_Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void VulkanTexture::TransitionToColorAttachment()
{
	device->TransitionImageLayout(
		m_Image,
		ConvertFormat(m_Format),
		ConvertAspectMask(m_AspectMask),
		m_Layout,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		m_MipLevels,
		m_LayerCount);

	m_Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}
