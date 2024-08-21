#pragma once

#include "../Core/Core.h"

#include <vulkan/vulkan.h>

namespace Pengine::Vk
{

	class PENGINE_API VulkanSamplerManager
	{
	public:
		static VulkanSamplerManager& GetInstance();

		VulkanSamplerManager(const VulkanSamplerManager&) = delete;
		VulkanSamplerManager& operator=(const VulkanSamplerManager&) = delete;

		VkSampler CreateSampler(const VkSamplerCreateInfo& createInfo);

		void ShutDown();

	private:
		VulkanSamplerManager() = default;
		~VulkanSamplerManager() = default;

		std::optional<VkSampler> Find(const VkSamplerCreateInfo& createInfo);

		struct SamplerInfo
		{
			VkSamplerCreateInfo createInfo;
			VkSampler sampler;
		};

		std::vector<SamplerInfo> m_SamplerInfos;
	};

}
