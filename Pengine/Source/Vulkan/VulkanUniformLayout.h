#pragma once

#include "../Core/Core.h"
#include "../Graphics/UniformLayout.h"

#include <vulkan/vulkan.h>

namespace Pengine
{

	namespace Vk
	{

		class PENGINE_API VulkanUniformLayout : public UniformLayout
		{
		public:
			VulkanUniformLayout(const std::unordered_map<uint32_t, Binding>& bindings);
			~VulkanUniformLayout();
			VulkanUniformLayout(const VulkanUniformLayout&) = delete;
			VulkanUniformLayout& operator=(const VulkanUniformLayout&) = delete;

			VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout[Vk::swapChainImageIndex]; }

			static VkDescriptorType ConvertDescriptorType(Type type);

			static Type ConvertDescriptorType(VkDescriptorType type);

			static VkShaderStageFlagBits ConvertDescriptorStage(Stage stage);

			static Stage ConvertDescriptorStage(VkShaderStageFlagBits stage);

		private:
			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_BindingsByLocation;
			std::vector<VkDescriptorSetLayout> m_DescriptorSetLayout;
		};

	}

}