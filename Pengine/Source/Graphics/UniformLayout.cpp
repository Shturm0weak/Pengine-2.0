#include "UniformLayout.h"

#include "../Core/Logger.h"
#include "../Utils/Utils.h"
#include "../Vulkan/VulkanUniformLayout.h"

using namespace Pengine;

std::shared_ptr<UniformLayout> UniformLayout::Create(
	const std::unordered_map<uint32_t, Binding>& bindings)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanUniformLayout>(bindings);
	}
}

UniformLayout::UniformLayout(const std::unordered_map<uint32_t, Binding>& bindings)
	: m_BindingsByLocation(bindings)
{
	for (auto [location, binding] : bindings)
	{
		m_BindingLocationsByName[binding.name] = location;
	}
}

uint32_t UniformLayout::GetBindingLocationByName(const std::string& name) const
{
	auto bindingLocationByName = m_BindingLocationsByName.find(name);
	if (bindingLocationByName != m_BindingLocationsByName.end())
	{
		return bindingLocationByName->second;
	}

	FATAL_ERROR("No location at this " + name +  " name!")
}

UniformLayout::Binding UniformLayout::GetBindingByLocation(uint32_t location) const
{
	auto bindingByLocation = m_BindingsByLocation.find(location);
	if (bindingByLocation != m_BindingsByLocation.end())
	{
		return bindingByLocation->second;
	}

	FATAL_ERROR("No binding at this " + std::to_string(location) + " location!")
}

UniformLayout::Binding UniformLayout::GetBindingByName(const std::string& name) const
{
	auto bindingLocationByName = m_BindingLocationsByName.find(name);
	if (bindingLocationByName != m_BindingLocationsByName.end())
	{
		return GetBindingByLocation(bindingLocationByName->second);
	}

	FATAL_ERROR("No binding at this " + name + " name!")
}

std::optional<UniformLayout::Variable> UniformLayout::Binding::GetValue(const std::string& name)
{
	for (const auto& value : values)
	{
		if (value.name == name)
		{
			return value;
		}
	}

	return std::nullopt;
}
