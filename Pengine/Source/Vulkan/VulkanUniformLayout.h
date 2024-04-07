#pragma once

#include "../Core/Core.h"
#include "../Graphics/UniformLayout.h"

#include <vulkan/vulkan.h>

namespace Pengine::Vk
{

	class PENGINE_API VulkanUniformLayout final: public UniformLayout
	{
	public:
		explicit VulkanUniformLayout(const std::vector<ShaderReflection::ReflectDescriptorSetBinding>& bindings);
		virtual ~VulkanUniformLayout() override;
		VulkanUniformLayout(const VulkanUniformLayout&) = delete;
		VulkanUniformLayout(VulkanUniformLayout&&) = delete;
		VulkanUniformLayout& operator=(const VulkanUniformLayout&) = delete;
		VulkanUniformLayout& operator=(VulkanUniformLayout&&) = delete;

		VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

		static VkDescriptorType ConvertDescriptorType(ShaderReflection::Type type);

		static ShaderReflection::Type ConvertDescriptorType(VkDescriptorType type);

		static VkShaderStageFlagBits ConvertDescriptorStage(ShaderReflection::Stage stage);

		static ShaderReflection::Stage ConvertDescriptorStage(VkShaderStageFlagBits stage);

	private:
		VkDescriptorSetLayout m_DescriptorSetLayout;
	};

}