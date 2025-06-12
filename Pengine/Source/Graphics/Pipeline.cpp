#include "Pipeline.h"

#include "../Core/Logger.h"
#include "../Utils/Utils.h"

using namespace Pengine;

Pipeline::Type Pipeline::GetPipelineType(const std::map<ShaderModule::Type, std::filesystem::path>& shaderFilepathsByType)
{
	bool isGraphicsPipeline = false;
	bool isComputePipeline = false;
	for (const auto& [type, filepath] : shaderFilepathsByType)
	{
		if (type == ShaderModule::Type::VERTEX ||
			type == ShaderModule::Type::FRAGMENT ||
			type == ShaderModule::Type::GEOMETRY)
		{
			isGraphicsPipeline = true;
		}

		if (type == ShaderModule::Type::COMPUTE)
		{
			isComputePipeline = true;
		}
	}

	if (isComputePipeline && isGraphicsPipeline)
	{
		FATAL_ERROR("Pipeline contains two different types, graphics and compute at the same time!");
	}
	else if (!isComputePipeline && !isGraphicsPipeline)
	{
		FATAL_ERROR("Pipeline does not contain any shader stage!");
	}

	Type type{};
	if (isGraphicsPipeline)
	{
		type = Type::GRAPHICS;
	}
	else if (isComputePipeline)
	{
		type = Type::COMPUTE;
	}

	return type;
}

Pipeline::Pipeline(Type type)
	: m_Type(type)
{
}

const std::optional<uint32_t> Pipeline::GetDescriptorSetIndexByType(const DescriptorSetIndexType type, const std::string& name) const
{
	std::map<std::string, uint32_t> descriptorSetsByRenderPass = GetDescriptorSetIndexByType(type);

	auto foundSet = descriptorSetsByRenderPass.find(name);
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
