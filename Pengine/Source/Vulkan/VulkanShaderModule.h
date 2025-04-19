#pragma once

#include "../Core/Core.h"

#include "../Graphics/ShaderModule.h"

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

namespace Pengine::Vk
{

	class PENGINE_API VulkanShaderModule final : public ShaderModule
	{
	public:
		VulkanShaderModule(
			const std::filesystem::path& filepath,
			const Type type);
		virtual ~VulkanShaderModule() override;
		VulkanShaderModule(const VulkanShaderModule&) = delete;
		VulkanShaderModule& operator=(const VulkanShaderModule&) = delete;

		[[nodiscard]] VkShaderModule GetShaderModule() const { return m_ShaderModule; }

	private:
		VkShaderModule m_ShaderModule = VK_NULL_HANDLE;
	};

}
