#include "VulkanUniformLayout.h"

#include "VulkanDevice.h"

#include "../Core/Logger.h"

using namespace Pengine;
using namespace Vk;

VulkanUniformLayout::VulkanUniformLayout(const std::vector<ShaderReflection::ReflectDescriptorSetBinding>& bindings)
	: UniformLayout(bindings)
{
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};

	for (const auto& binding : bindings)
	{
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding.binding;
		layoutBinding.descriptorType = ConvertDescriptorType(binding.type);
		layoutBinding.descriptorCount = binding.count;
		layoutBinding.pImmutableSamplers = 0;
		layoutBinding.stageFlags |= ConvertDescriptorStage(binding.stage);
		
		setLayoutBindings.emplace_back(layoutBinding);
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();
	descriptorSetLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

	const std::vector<VkDescriptorBindingFlags> bindingFlags(bindings.size(), VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
	VkDescriptorSetLayoutBindingFlagsCreateInfo descriptorSetLayoutBindingFlagsInfo{};
	descriptorSetLayoutBindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	descriptorSetLayoutBindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
	descriptorSetLayoutBindingFlagsInfo.pBindingFlags = bindingFlags.data();

	descriptorSetLayoutCreateInfo.pNext = &descriptorSetLayoutBindingFlagsInfo;

	if (vkCreateDescriptorSetLayout(
		device->GetDevice(),
		&descriptorSetLayoutCreateInfo,
		nullptr,
		&m_DescriptorSetLayout) != VK_SUCCESS)
	{
		FATAL_ERROR("failed to create descriptor set layout!");
	}
}

VulkanUniformLayout::~VulkanUniformLayout()
{
	vkDeviceWaitIdle(device->GetDevice());
	vkDestroyDescriptorSetLayout(device->GetDevice(), m_DescriptorSetLayout, nullptr);
}

VkDescriptorType VulkanUniformLayout::ConvertDescriptorType(const ShaderReflection::Type type)
{
	switch (type)
	{
		case Pengine::ShaderReflection::Type::SAMPLER:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case Pengine::ShaderReflection::Type::COMBINED_IMAGE_SAMPLER:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case Pengine::ShaderReflection::Type::SAMPLED_IMAGE:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case Pengine::ShaderReflection::Type::STORAGE_IMAGE:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case Pengine::ShaderReflection::Type::UNIFORM_TEXEL_BUFFER:
			return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		case Pengine::ShaderReflection::Type::STORAGE_TEXEL_BUFFER:
			return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		case Pengine::ShaderReflection::Type::UNIFORM_BUFFER:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case Pengine::ShaderReflection::Type::STORAGE_BUFFER:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case Pengine::ShaderReflection::Type::UNIFORM_BUFFER_DYNAMIC:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		case Pengine::ShaderReflection::Type::STORAGE_BUFFER_DYNAMIC:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		case Pengine::ShaderReflection::Type::INPUT_ATTACHMENT:
			return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		case Pengine::ShaderReflection::Type::ACCELERATION_STRUCTURE_KHR:
			return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	}

	FATAL_ERROR("Failed to convert descriptor type!");
	return VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

ShaderReflection::Type VulkanUniformLayout::ConvertDescriptorType(const VkDescriptorType type)
{
	switch (type)
	{
		case VK_DESCRIPTOR_TYPE_SAMPLER:
			return Pengine::ShaderReflection::Type::SAMPLER;
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			return Pengine::ShaderReflection::Type::COMBINED_IMAGE_SAMPLER;
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			return Pengine::ShaderReflection::Type::SAMPLED_IMAGE;
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			return Pengine::ShaderReflection::Type::STORAGE_IMAGE;
		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			return Pengine::ShaderReflection::Type::UNIFORM_TEXEL_BUFFER;
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			return Pengine::ShaderReflection::Type::STORAGE_TEXEL_BUFFER;
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			return Pengine::ShaderReflection::Type::UNIFORM_BUFFER;
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			return Pengine::ShaderReflection::Type::STORAGE_BUFFER;
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			return Pengine::ShaderReflection::Type::UNIFORM_BUFFER_DYNAMIC;
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			return Pengine::ShaderReflection::Type::STORAGE_BUFFER_DYNAMIC;
		case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			return Pengine::ShaderReflection::Type::INPUT_ATTACHMENT;
		case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
			return Pengine::ShaderReflection::Type::ACCELERATION_STRUCTURE_KHR;
	}

	FATAL_ERROR("Failed to convert descriptor type!");
	return {};
}

VkShaderStageFlagBits VulkanUniformLayout::ConvertDescriptorStage(const ShaderReflection::Stage stage)
{
	switch (stage)
	{
	case Pengine::ShaderReflection::Stage::VERTEX:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case Pengine::ShaderReflection::Stage::FRAGMENT:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	}

	FATAL_ERROR("Failed to convert descriptor stage!");
	return VkShaderStageFlagBits::VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
}

ShaderReflection::Stage VulkanUniformLayout::ConvertDescriptorStage(const VkShaderStageFlagBits stage)
{
	switch (stage)
	{
	case VK_SHADER_STAGE_VERTEX_BIT:
		return Pengine::ShaderReflection::Stage::VERTEX;
	case VK_SHADER_STAGE_FRAGMENT_BIT:
		return Pengine::ShaderReflection::Stage::FRAGMENT;
	}

	FATAL_ERROR("Failed to convert descriptor stage!");
	return {};
}
