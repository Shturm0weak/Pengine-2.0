#include "BaseMaterial.h"

#include "../Core/Logger.h"
#include "../Core/Serializer.h"

#include <algorithm>

using namespace Pengine;

std::shared_ptr<BaseMaterial> BaseMaterial::Create(const std::string& name, const std::string& filepath,
	const std::vector<Pipeline::CreateInfo>& pipelineCreateInfos)
{
	return std::make_shared<BaseMaterial>(name, filepath, pipelineCreateInfos);
}

std::shared_ptr<BaseMaterial> BaseMaterial::Load(const std::string& filepath)
{
	std::vector<Pipeline::CreateInfo> pipelineCreateInfos = Serializer::LoadBaseMaterial(filepath);

	return BaseMaterial::Create(filepath, filepath, pipelineCreateInfos);
}

BaseMaterial::BaseMaterial(const std::string& name, const std::string& filepath,
	const std::vector<Pipeline::CreateInfo>& pipelineCreateInfos)
	: Asset(name, filepath)
{
	for (const auto& pipelineCreateInfo : pipelineCreateInfos)
	{
		auto pipeline = Pipeline::Create(pipelineCreateInfo);
		m_PipelinesByRenderPass[pipelineCreateInfo.renderPass->GetType()] = pipeline;
	}
}

BaseMaterial::~BaseMaterial()
{
	m_PipelinesByRenderPass.clear();
}

std::shared_ptr<Pipeline> BaseMaterial::GetPipeline(const std::string& type) const
{
	auto pipelineByRenderPass = m_PipelinesByRenderPass.find(type);
	if (pipelineByRenderPass != m_PipelinesByRenderPass.end())
	{
		return pipelineByRenderPass->second;
	}
	
	return nullptr;
}
