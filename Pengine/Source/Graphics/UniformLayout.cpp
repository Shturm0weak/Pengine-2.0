#include "UniformLayout.h"

#include "../Core/Logger.h"
#include "../Utils/Utils.h"
#include "../Vulkan/VulkanUniformLayout.h"

using namespace Pengine;

std::shared_ptr<UniformLayout> UniformLayout::Create(
	const std::vector<ShaderReflection::ReflectDescriptorSetBinding>& bindings)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanUniformLayout>(bindings);
	}

	FATAL_ERROR("Failed to create the uniform layout, no graphics API implementation");
	return nullptr;
}

UniformLayout::UniformLayout(const std::vector<ShaderReflection::ReflectDescriptorSetBinding>& bindings)
	: m_Bindings(bindings)
{
}

std::optional<ShaderReflection::ReflectDescriptorSetBinding> UniformLayout::GetBindingByLocation(const uint32_t location) const
{
	for (const auto& binding : m_Bindings)
	{
		if (binding.binding == location)
		{
			return binding;
		}
	}

	return std::nullopt;
}

std::optional<ShaderReflection::ReflectDescriptorSetBinding> UniformLayout::GetBindingByName(const std::string& name) const
{
	for (const auto& binding : m_Bindings)
	{
		if (binding.name == name)
		{
			return binding;
		}
	}

	return std::nullopt;
}

uint32_t UniformLayout::GetBindingLocationByName(const std::string& name) const
{
	for (const auto& binding : m_Bindings)
	{
		if (binding.name == name)
		{
			return binding.binding;
		}
	}

	return -1;
}
