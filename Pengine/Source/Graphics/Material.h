#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"
#include "../Core/Logger.h"

#include "BaseMaterial.h"
#include "WriterBufferHelper.h"

#include "../Utils/Utils.h"

namespace Pengine
{

	class PENGINE_API Material final : public Asset
	{
	public:
		

		struct CreateInfo
		{
			std::unordered_map<std::string, Pipeline::UniformInfo> uniformInfos;
			std::shared_ptr<BaseMaterial> baseMaterial;
		};

		static std::shared_ptr<Material> Create(
			const std::string& name,
			const std::filesystem::path& filepath,
			const CreateInfo& createInfo);

		static std::shared_ptr<Material> Load(const std::filesystem::path& filepath);

		static void Save(const std::shared_ptr<Material>& material);

		static std::shared_ptr<Material> Clone(
			const std::string& name,
			const std::filesystem::path& filepath,
			const std::shared_ptr<Material>& material);

		Material(const std::string& name, const std::filesystem::path& filepath,
			const CreateInfo& createInfo);
		~Material() = default;
		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;

		std::shared_ptr<BaseMaterial> GetBaseMaterial() const { return m_BaseMaterial; }

		std::shared_ptr<UniformWriter> GetUniformWriter(const std::string& renderPassName) const;

		std::shared_ptr<Buffer> GetBuffer(const std::string& name) const;

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
		std::shared_ptr<BaseMaterial> m_BaseMaterial;
		std::unordered_map<std::string, std::shared_ptr<UniformWriter>> m_UniformWriterByRenderPass;
		std::unordered_map<std::string, std::shared_ptr<Buffer>> m_BuffersByName;
	};

	template<typename T>
	inline void Material::WriteToBuffer(
		const std::string& uniformBufferName,
		const std::string& valueName, 
		T& value)
	{
		WriterBufferHelper::WriteToBuffer(m_BaseMaterial.get(), GetBuffer(uniformBufferName), uniformBufferName, valueName, value);
	}

	template<typename T>
	inline T Material::GetBufferValue(const std::string& uniformBufferName, const std::string& valueName)
	{
		return WriterBufferHelper::GetBufferValue<T>(m_BaseMaterial.get(), GetBuffer(uniformBufferName), uniformBufferName, valueName);
	}

}
