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
	for (const auto& [type, setsByrenderPass] : pipelineCreateInfo.descriptorSetIndicesByType)
	{
		for (const auto& [renderPassName, set] : setsByrenderPass)
		{
			m_SortedDescriptorSets.emplace(set, std::make_pair(type, renderPassName));
		}
	}
}

std::map<std::string, uint32_t> Pipeline::GetDescriptorSetIndexByType(const DescriptorSetIndexType type) const
{
	return Utils::Find(type, m_CreateInfo.descriptorSetIndicesByType);
}

const std::optional<uint32_t> Pipeline::GetDescriptorSetIndexByType(const DescriptorSetIndexType type, const std::string& renderPassName) const
{
	std::map<std::string, uint32_t> descriptorSetsByRenderPass = GetDescriptorSetIndexByType(type);
	
	auto foundSet = descriptorSetsByRenderPass.find(renderPassName);
	if (foundSet != descriptorSetsByRenderPass.end())
	{
		return foundSet->second;
	}

	return std::nullopt;
}

std::shared_ptr<UniformLayout> Pipeline::GetUniformLayout(const uint32_t descriptorSet) const
{
	return Utils::Find(descriptorSet, m_UniformLayoutsByDescriptorSet);
}
