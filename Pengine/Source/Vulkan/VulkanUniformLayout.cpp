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
		layoutBinding.stageFlags = ConvertStage(binding.stage);
		
		setLayoutBindings.emplace_back(layoutBinding);
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();

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
	device->DeleteResource([descriptorSetLayout = m_DescriptorSetLayout]()
	{
		vkDestroyDescriptorSetLayout(device->GetDevice(), descriptorSetLayout, nullptr);
	});
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

VkShaderStageFlags VulkanUniformLayout::ConvertStage(ShaderReflection::Stage stage)
{
	switch (stage)
	{
		case ShaderReflection::Stage::VERTEX_BIT:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderReflection::Stage::TESSELLATION_CONTROL_BIT:
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case ShaderReflection::Stage::TESSELLATION_EVALUATION_BIT:
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case ShaderReflection::Stage::GEOMETRY_BIT:
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderReflection::Stage::FRAGMENT_BIT:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderReflection::Stage::COMPUTE_BIT:
			return VK_SHADER_STAGE_COMPUTE_BIT;
		case ShaderReflection::Stage::ALL_GRAPHICS:
			return VK_SHADER_STAGE_ALL_GRAPHICS;
		case ShaderReflection::Stage::ALL:
			return VK_SHADER_STAGE_ALL;
	}

	FATAL_ERROR("Failed to convert shader stage!");
	return {};
}

ShaderReflection::Stage VulkanUniformLayout::ConvertStage(VkShaderStageFlags stage)
{
	switch (stage)
	{
	case VK_SHADER_STAGE_VERTEX_BIT:
		return ShaderReflection::Stage::VERTEX_BIT;
	case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
		return ShaderReflection::Stage::TESSELLATION_CONTROL_BIT;
	case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
		return ShaderReflection::Stage::TESSELLATION_EVALUATION_BIT;
	case VK_SHADER_STAGE_GEOMETRY_BIT:
		return ShaderReflection::Stage::GEOMETRY_BIT;
	case VK_SHADER_STAGE_FRAGMENT_BIT:
		return ShaderReflection::Stage::FRAGMENT_BIT;
	case VK_SHADER_STAGE_COMPUTE_BIT:
		return ShaderReflection::Stage::COMPUTE_BIT;
	case VK_SHADER_STAGE_ALL_GRAPHICS:
		return ShaderReflection::Stage::ALL_GRAPHICS;
	case VK_SHADER_STAGE_ALL:
		return ShaderReflection::Stage::ALL;
	}

	FATAL_ERROR("Failed to convert shader stage!");
	return {};
}
