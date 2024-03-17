#pragma once

#include "../Core/Core.h"
#include "../Graphics/UniformLayout.h"

#include <vulkan/vulkan.h>

namespace Pengine::Vk
{

	class PENGINE_API VulkanUniformLayout final: public UniformLayout
	{
	public:
		explicit VulkanUniformLayout(const std::unordered_map<uint32_t, Binding>& bindings);
		virtual ~VulkanUniformLayout() override;
		VulkanUniformLayout(const VulkanUniformLayout&) = delete;
		VulkanUniformLayout(VulkanUniformLayout&&) = delete;
		VulkanUniformLayout& operator=(const VulkanUniformLayout&) = delete;
		VulkanUniformLayout& operator=(VulkanUniformLayout&&) = delete;

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