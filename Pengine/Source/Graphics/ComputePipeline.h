#pragma once

#include "../Core/Core.h"

#include "Pipeline.h"

namespace Pengine
{

	class PENGINE_API ComputePipeline : public Pipeline
	{
	  public:
		struct CreateComputeInfo
		{
			Type type;
			std::string passName;
			std::map<DescriptorSetIndexType, std::map<std::string, uint32_t>> descriptorSetIndicesByType;
			std::map<Pipeline::ShaderType, std::string> shaderFilepathsByType;
			UniformInfo uniformInfo;
		};

		static std::shared_ptr<Pipeline> Create(const CreateComputeInfo& createComputeInfo);

		explicit ComputePipeline(const CreateComputeInfo& createComputeInfo);
		virtual ~ComputePipeline() override = default;
		ComputePipeline(const ComputePipeline&) = delete;
		ComputePipeline(ComputePipeline&&) = delete;
		ComputePipeline& operator=(const ComputePipeline&) = delete;
		ComputePipeline& operator=(ComputePipeline&&) = delete;

		const CreateComputeInfo& GetCreateInfo() const { return m_CreateInfo; }

		virtual const UniformInfo& GetUniformInfo() const override { return m_CreateInfo.uniformInfo; }

		virtual std::map<std::string, uint32_t> GetDescriptorSetIndexByType(const DescriptorSetIndexType type) const override;

	  private:
		CreateComputeInfo m_CreateInfo;
	};

}
