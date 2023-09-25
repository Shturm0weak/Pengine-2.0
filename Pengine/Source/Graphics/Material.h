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

}