#include "VulkanUniformLayout.h"

#include "VulkanDevice.h"

#include "../Core/Logger.h"

using namespace Pengine;
using namespace Vk;

VulkanUniformLayout::VulkanUniformLayout(const std::unordered_map<uint32_t, Binding>& bindings)
	: UniformLayout(bindings)
{
    m_DescriptorSetLayout.resize(Vk::swapChainImageCount);

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};

    for (const auto& binding : bindings)
	{
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding.first;
        layoutBinding.descriptorType = ConvertDescriptorType(binding.second.type);
        layoutBinding.descriptorCount = 1; // TODO: here was check for sampler array
        layoutBinding.pImmutableSamplers = 0;
        for (const auto& stage : binding.second.stages)
        {
            layoutBinding.stageFlags |= ConvertDescriptorStage(stage);
        }
        
        m_BindingsByLocation[binding.first] = layoutBinding;
        setLayoutBindings.emplace_back(layoutBinding);
	}

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();

    for (auto& descriptorSetLayout : m_DescriptorSetLayout)
    {
        if (vkCreateDescriptorSetLayout(
            device->GetDevice(),
            &descriptorSetLayoutCreateInfo,
            nullptr,
            &descriptorSetLayout) != VK_SUCCESS)
        {
            FATAL_ERROR("failed to create descriptor set layout!");
        }
    }
}

VulkanUniformLayout::~VulkanUniformLayout()
{
    vkDeviceWaitIdle(device->GetDevice());

    for (auto& descriptorSetLayout : m_DescriptorSetLayout)
    {
        vkDestroyDescriptorSetLayout(device->GetDevice(), descriptorSetLayout, nullptr);
    }
}

VkDescriptorType VulkanUniformLayout::ConvertDescriptorType(const Type type)
{
    switch (type)
    {
        case Pengine::UniformLayout::Type::SAMPLER:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case Pengine::UniformLayout::Type::COMBINED_IMAGE_SAMPLER:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case Pengine::UniformLayout::Type::SAMPLED_IMAGE:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case Pengine::UniformLayout::Type::STORAGE_IMAGE:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case Pengine::UniformLayout::Type::UNIFORM_TEXEL_BUFFER:
            return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case Pengine::UniformLayout::Type::STORAGE_TEXEL_BUFFER:
            return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        case Pengine::UniformLayout::Type::UNIFORM_BUFFER:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case Pengine::UniformLayout::Type::STORAGE_BUFFER:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case Pengine::UniformLayout::Type::UNIFORM_BUFFER_DYNAMIC:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case Pengine::UniformLayout::Type::STORAGE_BUFFER_DYNAMIC:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        case Pengine::UniformLayout::Type::INPUT_ATTACHMENT:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case Pengine::UniformLayout::Type::ACCELERATION_STRUCTURE_KHR:
            return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    }

    FATAL_ERROR("Failed to convert descriptor type!");
	return VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

UniformLayout::Type VulkanUniformLayout::ConvertDescriptorType(const VkDescriptorType type)
{
    switch (type)
    {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
            return Pengine::UniformLayout::Type::SAMPLER;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            return Pengine::UniformLayout::Type::COMBINED_IMAGE_SAMPLER;
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return Pengine::UniformLayout::Type::SAMPLED_IMAGE;
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return Pengine::UniformLayout::Type::STORAGE_IMAGE;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return Pengine::UniformLayout::Type::UNIFORM_TEXEL_BUFFER;
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return Pengine::UniformLayout::Type::STORAGE_TEXEL_BUFFER;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return Pengine::UniformLayout::Type::UNIFORM_BUFFER;
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return Pengine::UniformLayout::Type::STORAGE_BUFFER;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return Pengine::UniformLayout::Type::UNIFORM_BUFFER_DYNAMIC;
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return Pengine::UniformLayout::Type::STORAGE_BUFFER_DYNAMIC;
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return Pengine::UniformLayout::Type::INPUT_ATTACHMENT;
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
            return Pengine::UniformLayout::Type::ACCELERATION_STRUCTURE_KHR;
    }

    FATAL_ERROR("Failed to convert descriptor type!");
	return {};
}

VkShaderStageFlagBits VulkanUniformLayout::ConvertDescriptorStage(const Stage stage)
{
    switch (stage)
    {
    case Pengine::UniformLayout::Stage::VERTEX:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case Pengine::UniformLayout::Stage::FRAGMENT:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    FATAL_ERROR("Failed to convert descriptor stage!");
	return VkShaderStageFlagBits::VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
}

UniformLayout::Stage VulkanUniformLayout::ConvertDescriptorStage(const VkShaderStageFlagBits stage)
{
    switch (stage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT:
        return Pengine::UniformLayout::Stage::VERTEX;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return Pengine::UniformLayout::Stage::FRAGMENT;
    }

    FATAL_ERROR("Failed to convert descriptor stage!");
	return {};
}
