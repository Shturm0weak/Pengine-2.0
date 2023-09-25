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
        layoutBinding.descriptorCount = binding.second.type == Type::SAMPLER_ARRAY ? MAX_TEXTURES : 1;
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
            FATAL_ERROR("failed to create descriptor set layout!")
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

VkDescriptorType VulkanUniformLayout::ConvertDescriptorType(Type type)
{
    switch (type)
    {
    case Pengine::UniformLayout::Type::SAMPLER_ARRAY:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case Pengine::UniformLayout::Type::SAMPLER:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case Pengine::UniformLayout::Type::BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }

    FATAL_ERROR("Failed to convert descriptor type!")
}

UniformLayout::Type VulkanUniformLayout::ConvertDescriptorType(VkDescriptorType type)
{
    switch (type)
    {
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        return Pengine::UniformLayout::Type::SAMPLER;
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return Pengine::UniformLayout::Type::BUFFER;
    }

    FATAL_ERROR("Failed to convert descriptor type!")
}

VkShaderStageFlagBits VulkanUniformLayout::ConvertDescriptorStage(Stage stage)
{
    switch (stage)
    {
    case Pengine::UniformLayout::Stage::VERTEX:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case Pengine::UniformLayout::Stage::FRAGMENT:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    FATAL_ERROR("Failed to convert descriptor stage!")
}

UniformLayout::Stage VulkanUniformLayout::ConvertDescriptorStage(VkShaderStageFlagBits stage)
{
    switch (stage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT:
        return Pengine::UniformLayout::Stage::VERTEX;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return Pengine::UniformLayout::Stage::FRAGMENT;
    }

    FATAL_ERROR("Failed to convert descriptor stage!")
}
