#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"
#include "../Core/TextureSlots.h"

#include "Pipeline.h"
#include "UniformWriter.h"

namespace Pengine
{

	class PENGINE_API BaseMaterial : public Asset
	{
	public:
		static std::shared_ptr<BaseMaterial> Create(const std::string& name, const std::string& filepath, 
			const std::vector<Pipeline::CreateInfo>& pipelineCreateInfos);

		static std::shared_ptr<BaseMaterial> Load(const std::string& filepath);

		BaseMaterial(const std::string& name, const std::string& filepath,
			const std::vector<Pipeline::CreateInfo>& pipelineCreateInfos);
		~BaseMaterial();
		BaseMaterial(const BaseMaterial&) = delete;
		BaseMaterial& operator=(const BaseMaterial&) = delete;

		std::shared_ptr<Pipeline> GetPipeline(const std::string& type) const;

		std::unordered_map<std::string, std::shared_ptr<Pipeline>> GetPipelinesByRenderPass() const { return m_PipelinesByRenderPass; }

		size_t m_MaterialsSize = 0;

	private:
		std::unordered_map<std::string, std::shared_ptr<Pipeline>> m_PipelinesByRenderPass;
	};

}