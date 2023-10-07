#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"
#include "../Core/TextureSlots.h"

#include "Pipeline.h"
#include "UniformWriter.h"
#include "UniformLayout.h"

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

		template<typename T>
		void SetValue(const std::string& bufferName, const std::string& name, T& value);

	private:
		std::unordered_map<std::string, std::shared_ptr<Pipeline>> m_PipelinesByRenderPass;
	};

	template<typename T>
	inline void BaseMaterial::SetValue(const std::string& bufferName, const std::string& name, T& value)
	{
		for (const auto& [renderPass, pipeline] : m_PipelinesByRenderPass)
		{
			if (!pipeline->GetUniformWriter())
			{
				continue;
			}

			const UniformLayout::Binding& binding = pipeline->GetUniformWriter()->GetLayout()->GetBindingByName(bufferName);
			if (binding.type == UniformLayout::Type::BUFFER)
			{
				if (auto variable = binding.GetValue(name))
				{
					std::shared_ptr<Buffer> buffer = pipeline->GetBuffer(bufferName);
					void* data = buffer->GetData();
					Utils::SetValue(data, variable->offset, value);
				}
			}
		}
	}

}