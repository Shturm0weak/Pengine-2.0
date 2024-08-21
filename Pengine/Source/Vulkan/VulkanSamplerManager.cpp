#include "VulkanSamplerManager.h"

#include "../Core/Logger.h"

#include "VulkanDevice.h"

namespace Pengine::Vk
{

	VulkanSamplerManager& VulkanSamplerManager::GetInstance()
	{
		static VulkanSamplerManager vulkanSamplerManager;
		return vulkanSamplerManager;
	}

	VkSampler VulkanSamplerManager::CreateSampler(const VkSamplerCreateInfo& createInfo)
	{
		std::optional<VkSampler> foundSampler = Find(createInfo);

		if (!foundSampler)
		{
			SamplerInfo samplerInfo{};
			samplerInfo.createInfo = createInfo;

			if (vkCreateSampler(device->GetDevice(), &createInfo, nullptr, &samplerInfo.sampler) != VK_SUCCESS)
			{
				FATAL_ERROR("Failed to create texture sampler!");
			}

			m_SamplerInfos.emplace_back(samplerInfo);

			return samplerInfo.sampler;
		}

		return *foundSampler;
	}

	void VulkanSamplerManager::ShutDown()
	{
		for (auto& samplerInfo : m_SamplerInfos)
		{
			vkDestroySampler(device->GetDevice(), samplerInfo.sampler, nullptr);
		}
	}

	std::optional<VkSampler> VulkanSamplerManager::Find(const VkSamplerCreateInfo& createInfo)
	{
		// Note: very debatable, need to find a better way to cache samplers.
		for (const auto& samplerInfo : m_SamplerInfos)
		{
			if (samplerInfo.createInfo.addressModeU == createInfo.addressModeU
				&& samplerInfo.createInfo.addressModeV == createInfo.addressModeV
				&& samplerInfo.createInfo.addressModeW == createInfo.addressModeW
				&& samplerInfo.createInfo.anisotropyEnable == createInfo.anisotropyEnable
				&& samplerInfo.createInfo.borderColor == createInfo.borderColor
				&& samplerInfo.createInfo.compareEnable == createInfo.compareEnable
				&& samplerInfo.createInfo.compareOp == createInfo.compareOp
				&& samplerInfo.createInfo.flags == createInfo.flags
				&& samplerInfo.createInfo.magFilter == createInfo.magFilter
				&& samplerInfo.createInfo.maxAnisotropy == createInfo.maxAnisotropy
				&& samplerInfo.createInfo.maxLod == createInfo.maxLod
				&& samplerInfo.createInfo.minFilter == createInfo.minFilter
				&& samplerInfo.createInfo.minLod == createInfo.minLod
				&& samplerInfo.createInfo.mipLodBias == createInfo.mipLodBias
				&& samplerInfo.createInfo.mipmapMode == createInfo.mipmapMode
				&& samplerInfo.createInfo.unnormalizedCoordinates == createInfo.unnormalizedCoordinates)
			{
				return samplerInfo.sampler;
			}
		}

		return std::nullopt;
	}

}
