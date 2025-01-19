#pragma once

#include "../Core/Core.h"
#include "../Graphics/Texture.h"

#include <vma/vk_mem_alloc.h>

namespace Pengine::Vk
{

	class PENGINE_API VulkanTexture final : public Texture
	{
	public:
		explicit VulkanTexture(const CreateInfo& createInfo);
		virtual ~VulkanTexture() override;
		VulkanTexture(const VulkanTexture&) = delete;
		VulkanTexture(VulkanTexture&&) = delete;
		VulkanTexture& operator=(const VulkanTexture&) = delete;
		VulkanTexture& operator=(VulkanTexture&&) = delete;

		VkDescriptorImageInfo GetDescriptorInfo();

		static VkImageView CreateImageView(
			VkImage image,
			VkFormat format,
			VkImageAspectFlagBits aspectMask,
			uint32_t mipLevels,
			uint32_t layerCount,
			VkImageViewType imageViewType);

		static VkSampler CreateSampler(const Texture::SamplerCreateInfo& samplerCreateInfo);

		static VkImageAspectFlagBits ConvertAspectMask(AspectMask aspectMask);

		static AspectMask ConvertAspectMask(VkImageAspectFlagBits aspectMask);

		static VkImageLayout ConvertLayout(Layout layout);

		static Layout ConvertLayout(VkImageLayout layout);

		static VkImageUsageFlagBits ConvertUsage(Usage usage);

		static Usage ConvertUsage(VkImageUsageFlagBits usage);

		static VkSamplerAddressMode ConvertAddressMode(SamplerCreateInfo::AddressMode addressMode);

		static SamplerCreateInfo::AddressMode ConvertAddressMode(VkSamplerAddressMode addressMode);

		static VkFilter ConvertFilter(SamplerCreateInfo::Filter filter);

		static SamplerCreateInfo::Filter ConvertFilter(VkFilter filter);

		static VkSamplerMipmapMode ConvertMipmapMode(SamplerCreateInfo::MipmapMode mipmapMode);

		static SamplerCreateInfo::MipmapMode ConvertMipmapMode(VkSamplerMipmapMode mipmapMode);

		static VkBorderColor ConvertBorderColor(SamplerCreateInfo::BorderColor borderColor);

		static SamplerCreateInfo::BorderColor ConvertBorderColor(VkBorderColor borderColor);

		[[nodiscard]] virtual void* GetId() const override { return (void*)m_DescriptorSet; }

		virtual void GenerateMipMaps(void* frame = nullptr) override;

		virtual void Copy(std::shared_ptr<Texture> src, void* frame = nullptr) override;

		[[nodiscard]] VkImageView GetImageView() const { return m_View; }

		[[nodiscard]] VkImage GetImage() const { return m_Image; }

		[[nodiscard]] VkSampler GetSampler() const { return m_Sampler; }

		[[nodiscard]] VkImageLayout GetLayout() const { return m_Layout; }

		void TransitionToDst(VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

		void TransitionToSrc(VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

		void TransitionToRead(VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

		void TransitionToPrevious(VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

		void TransitionToColorAttachment(VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

	private:
		VkSampler m_Sampler{};
		VkImage m_Image{};
		VmaAllocation m_VmaAllocation = VK_NULL_HANDLE;
		VmaAllocationInfo m_VmaAllocationInfo{};
		VkImageView m_View{};
		VkDescriptorSet m_DescriptorSet{};
		VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout m_PreviousLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

}