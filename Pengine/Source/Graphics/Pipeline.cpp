#include "Pipeline.h"

#include "../Core/Logger.h"
#include "../Utils/Utils.h"
#include "../Vulkan/VulkanPipeline.h"

using namespace Pengine;

std::shared_ptr<Pipeline> Pipeline::Create(const CreateInfo& pipelineCreateInfo)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanPipeline>(pipelineCreateInfo);
	}

	FATAL_ERROR("Failed to create the pipeline, no graphics API implementation");
	return nullptr;
}

Pipeline::Pipeline(const CreateInfo& pipelineCreateInfo)
	: m_CreateInfo(pipelineCreateInfo)
{
}

std::shared_ptr<UniformLayout> Pipeline::GetUniformLayout(const uint32_t descriptorSet) const
{
	auto uniformLayout = m_UniformLayoutsByDescriptorSet.find(descriptorSet);
	if (uniformLayout != m_UniformLayoutsByDescriptorSet.end())
	{
		return uniformLayout->second;
	}

	return nullptr;
}

const std::optional<uint32_t> Pipeline::GetDescriptorSetIndexByType(const DescriptorSetIndexType type) const
{
	auto descriptorSetIndexByType = m_CreateInfo.descriptorSetIndicesByType.find(type);
	if (descriptorSetIndexByType != m_CreateInfo.descriptorSetIndicesByType.end())
	{
		return descriptorSetIndexByType->second;
	}

	return std::nullopt;
}
