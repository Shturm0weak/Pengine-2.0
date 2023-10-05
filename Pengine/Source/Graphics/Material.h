#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"

#include "BaseMaterial.h"
#include "../Utils/Utils.h"

namespace Pengine
{

	class PENGINE_API Material : public Asset
	{
	public:
		struct UniformBufferInfo
		{
			std::unordered_map<std::string, int> intValuesByName;
			std::unordered_map<std::string, float> floatValuesByName;
			std::unordered_map<std::string, std::string> samplerValuesByName;
			std::unordered_map<std::string, glm::vec2> vec2ValuesByName;
			std::unordered_map<std::string, glm::vec3> vec3ValuesByName;
			std::unordered_map<std::string, glm::vec4> vec4ValuesByName;
		};

		struct RenderPassInfo
		{
			std::unordered_map<std::string, UniformBufferInfo> uniformBuffersByName;
			std::unordered_map<std::string, std::string> texturesByName;
		};

		struct CreateInfo
		{
			std::unordered_map<std::string, RenderPassInfo> renderPassInfo;
			std::shared_ptr<BaseMaterial> baseMaterial;
		};

		static std::shared_ptr<Material> Create(const std::string& name, const std::string& filepath,
			const CreateInfo& createInfo);

		static std::shared_ptr<Material> Load(const std::string& filepath);

		static void Save(std::shared_ptr<Material> material);

		static std::shared_ptr<Material> Inherit(const std::string& name, const std::string& filepath,
			std::shared_ptr<BaseMaterial> baseMaterial);

		static std::shared_ptr<Material> Clone(const std::string& name, const std::string& filepath,
			std::shared_ptr<Material> material);

		Material(const std::string& name, const std::string& filepath,
			const CreateInfo& createInfo);
		~Material();
		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;

		std::shared_ptr<BaseMaterial> GetBaseMaterial() const { return m_BaseMaterial; }

		std::shared_ptr<UniformWriter> GetUniformWriter(const std::string& renderPassName) const;

		std::shared_ptr<Texture> GetTexture(const std::string& name) const;

		void SetTexture(const std::string& name, std::shared_ptr<Texture> texture);

		const std::unordered_map<std::string, std::shared_ptr<class Texture>>& GetTextures() const { return m_TexturesByName; }

		int const GetIndex() const { return m_Index; }

		template<typename T>
		void SetValue(const std::string& bufferName, const std::string& name, T& value);

		struct RenderPassAttachmentInfo
		{
			std::string renderPassType;
			int attachment;
		};
		std::unordered_map<std::string, RenderPassAttachmentInfo> m_RenderPassAttachments;

	private:
		std::unordered_map<std::string, std::shared_ptr<class Texture>> m_TexturesByName;

		std::shared_ptr<BaseMaterial> m_BaseMaterial;
		std::unordered_map<std::string, std::shared_ptr<UniformWriter>> m_UniformWriterByRenderPass;

		int m_Index = 0;
	};

	template<typename T>
	inline void Material::SetValue(const std::string& bufferName, const std::string& name, T& value)
	{
		for (const auto& [renderPass, uniformWriter] : m_UniformWriterByRenderPass)
		{
			const UniformLayout::Binding& binding = uniformWriter->GetLayout()->GetBindingByName(bufferName);
			if (binding.type == UniformLayout::Type::BUFFER)
			{
				if (auto variable = binding.GetValue(name))
				{
					std::shared_ptr<Buffer> buffer = m_BaseMaterial->GetPipeline(renderPass)->GetBuffer(bufferName);
					void* data = (char*)buffer->GetData() + buffer->GetInstanceSize() * m_Index;
					Utils::SetValue(data, variable->offset, value);
				}
			}
		}
	}

}