#include "ComputePipeline.h"

#include "../Core/Logger.h"

#include "../Utils/Utils.h"

#include "../Vulkan/VulkanComputePipeline.h"

using namespace Pengine;

std::shared_ptr<Pipeline> ComputePipeline::Create(const CreateComputeInfo& createComputeInfo)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanComputePipeline>(createComputeInfo);
	}

	FATAL_ERROR("Failed to create the pipeline, no graphics API implementation");
	return nullptr;
}

ComputePipeline::ComputePipeline(const CreateComputeInfo& createComputeInfo)
	: Pipeline(createComputeInfo.type)
	, m_CreateInfo(createComputeInfo)
{
	for (const auto& [type, setsByRenderPass] : createComputeInfo.descriptorSetIndicesByType)
	{
		for (const auto& [renderPassName, set] : setsByRenderPass)
		{
			m_SortedDescriptorSets.emplace(set, std::make_pair(type, renderPassName));
		}
	}
}

std::map<std::string, uint32_t> ComputePipeline::GetDescriptorSetIndexByType(
	const Pipeline::DescriptorSetIndexType type) const
{
	return Utils::Find(type, m_CreateInfo.descriptorSetIndicesByType);
}
