#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"

#include "Pipeline.h"
#include "UniformWriter.h"
#include "UniformLayout.h"
#include "WriterBufferHelper.h"

#include "../Utils/Utils.h"

namespace Pengine
{

	class PENGINE_API BaseMaterial final : public Asset
	{
	public:
		struct CreateInfo
		{
			std::vector<Pipeline::CreateInfo> pipelineCreateInfos;
		};

		static std::shared_ptr<BaseMaterial> Create(
			const std::string& name,
			const std::filesystem::path& filepath,
			const CreateInfo& createInfo);

		static std::shared_ptr<BaseMaterial> Load(const std::filesystem::path& filepath);

		BaseMaterial(
			const std::string& name,
			const std::filesystem::path& filepath,
			const CreateInfo& createInfo);
		~BaseMaterial();
		BaseMaterial(const BaseMaterial&) = delete;
		BaseMaterial& operator=(const BaseMaterial&) = delete;

		std::shared_ptr<Pipeline> GetPipeline(const std::string& renderPassName) const;

		std::unordered_map<std::string, std::shared_ptr<Pipeline>> GetPipelinesByRenderPass() const { return m_PipelinesByRenderPass; }

		std::shared_ptr<UniformWriter> GetUniformWriter(const std::string& renderPassName) const;

		std::shared_ptr<Buffer> GetBuffer(const std::string& name) const;

		const std::unordered_map<std::string, UniformLayout::RenderTargetInfo>& GetRenderTargetsByName() const { return m_RenderTargetsByName; }

		bool GetUniformDetails(
			const std::string& uniformBufferName,
			const std::string& valueName,
			uint32_t& size,
			uint32_t& offset) const;

		std::optional<ShaderReflection::ReflectVariable> GetUniformValue(
			const std::string& uniformBufferName,
			const std::string& valueName);

		template<typename T>
		void WriteToBuffer(
			const std::string& uniformBufferName,
			const std::string& valueName,
			T& value);

		template<typename T>
		T GetBufferValue(
			const std::string& uniformBufferName,
			const std::string& valueName);

	private:
		std::unordered_map<std::string, std::shared_ptr<Pipeline>> m_PipelinesByRenderPass;
		std::unordered_map<std::string, std::shared_ptr<UniformWriter>> m_UniformWriterByRenderPass;
		std::unordered_map<std::string, UniformLayout::RenderTargetInfo> m_RenderTargetsByName;
		std::unordered_map<std::string, std::shared_ptr<Buffer>> m_BuffersByName;
	};

	template<typename T>
	inline void BaseMaterial::WriteToBuffer(
		const std::string& uniformBufferName,
		const std::string& valueName,
		T& value)
	{
		WriterBufferHelper::WriteToBuffer(this, GetBuffer(uniformBufferName), uniformBufferName, valueName, value);
	}

	template<typename T>
	inline T BaseMaterial::GetBufferValue(const std::string& uniformBufferName, const std::string& valueName)
	{
		WriterBufferHelper::GetBufferValue<T>(this, GetBuffer(uniformBufferName), uniformBufferName, valueName);
	}

}
