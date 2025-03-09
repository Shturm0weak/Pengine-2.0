#include "GraphicsPipeline.h"

#include "../Core/Logger.h"

#include "../Utils/Utils.h"

#include "../Vulkan/VulkanGraphicsPipeline.h"

using namespace Pengine;

std::shared_ptr<Pipeline> GraphicsPipeline::Create(const GraphicsPipeline::CreateGraphicsInfo& createGraphicsInfo)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanGraphicsPipeline>(createGraphicsInfo);
	}

	FATAL_ERROR("Failed to create the pipeline, no graphics API implementation");
	return nullptr;
}

GraphicsPipeline::GraphicsPipeline(const GraphicsPipeline::CreateGraphicsInfo& createGraphicsInfo)
	: Pipeline(createGraphicsInfo.type)
	, m_CreateInfo(createGraphicsInfo)
{
	for (const auto& [type, setsByRenderPass] : createGraphicsInfo.descriptorSetIndicesByType)
	{
		for (const auto& [renderPassName, set] : setsByRenderPass)
		{
			m_SortedDescriptorSets.emplace(set, std::make_pair(type, renderPassName));
		}
	}
}

std::map<std::string, uint32_t> GraphicsPipeline::GetDescriptorSetIndexByType(
	const Pipeline::DescriptorSetIndexType type) const
{
	return Utils::Find(type, m_CreateInfo.descriptorSetIndicesByType);
}
