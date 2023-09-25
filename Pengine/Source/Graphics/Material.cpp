#include "Material.h"

#include "../Core/Serializer.h"
#include "../Core/TextureManager.h"
#include "../Core/ViewportManager.h"
#include "../Core/Logger.h"

#include "Renderer.h"

using namespace Pengine;

std::shared_ptr<Material> Material::Create(const std::string& name, const std::string& filepath,
	const CreateInfo& createInfo)
{
	return std::make_shared<Material>(name, filepath, createInfo);
}

std::shared_ptr<Material> Material::Load(const std::string& filepath)
{
	return Material::Create(filepath, filepath, Serializer::LoadMaterial(filepath));
}

Material::Material(const std::string& name, const std::string& filepath,
	const CreateInfo& createInfo)
	: Asset(name, filepath)
	, m_BaseMaterial(createInfo.baseMaterial)
{
	m_Index = m_BaseMaterial->m_MaterialsSize;
	m_BaseMaterial->m_MaterialsSize++;

	for (const auto& [renderPass, renderPassInfo] : createInfo.renderPassInfo)
	{
		if (renderPassInfo.texturesByName.empty() && renderPassInfo.uniformBuffersByName.empty())
		{
			continue;
		}

		std::shared_ptr<Pipeline> pipeline = m_BaseMaterial->GetPipeline(renderPass);
		std::shared_ptr<UniformLayout> uniformLayout = pipeline->GetChildUniformLayout();
		std::shared_ptr<UniformWriter> uniformWriter = UniformWriter::Create(uniformLayout);
		m_UniformWriterByRenderPass[renderPass] = uniformWriter;

		for (const auto& [name, filepath] : renderPassInfo.texturesByName)
		{
			m_TexturesByName[name] = TextureManager::GetInstance().Load(filepath);
		}

		for (const auto& [bufferName, bufferInfo] : renderPassInfo.uniformBuffersByName)
		{
			std::shared_ptr<Buffer> buffer = pipeline->GetBuffer(bufferName);
			UniformLayout::Binding binding = uniformLayout->GetBindingByName(bufferName);

			uniformWriter->WriteBuffer(bufferName, buffer, buffer->GetInstanceSize(), buffer->GetInstanceSize() * m_Index);

			void* data = (char*)buffer->GetData() + buffer->GetInstanceSize() * m_Index;

			for (const auto& value : binding.values)
			{
				if (value.type == "float")
				{
					for (auto const& [loadedValueName, loadedValue] : bufferInfo.floatValuesByName)
					{
						if (value.name == loadedValueName)
						{
							Utils::GetValue<float>(data, value.offset) = loadedValue;
						}
					}
				}

				if (value.type == "int")
				{
					for (auto const& [loadedValueName, loadedValue] : bufferInfo.intValuesByName)
					{
						if (value.name == loadedValueName)
						{
							Utils::GetValue<int>(data, value.offset) = loadedValue;
						}
					}
				}

				if (value.type == "vec4" || value.type == "color")
				{
					for (auto const& [loadedValueName, loadedValue] : bufferInfo.vec4ValuesByName)
					{
						if (value.name == loadedValueName)
						{
							Utils::GetValue<glm::vec4>(data, value.offset) = loadedValue;
						}
					}
				}

				if (value.type == "vec3")
				{
					for (auto const& [loadedValueName, loadedValue] : bufferInfo.vec3ValuesByName)
					{
						if (value.name == loadedValueName)
						{
							Utils::GetValue<glm::vec3>(data, value.offset) = loadedValue;
						}
					}
				}

				if (value.type == "vec2")
				{
					for (auto const& [loadedValueName, loadedValue] : bufferInfo.vec2ValuesByName)
					{
						if (value.name == loadedValueName)
						{
							Utils::GetValue<glm::vec2>(data, value.offset) = loadedValue;
						}
					}
				}

				if (value.type == "sampler")
				{
					for (auto const& [loadedValueName, loadedValue] : bufferInfo.samplerValuesByName)
					{
						if (value.name == loadedValueName)
						{
							if (Utils::Contains(loadedValue, "RenderPass"))
							{
								RenderPassAttachmentInfo renderPassAttachmentInfo{};

								std::string renderPassType = Utils::EraseFromFront(loadedValue, '/');
								renderPassAttachmentInfo.attachment = std::stoi(Utils::EraseFromFront(renderPassType, '/'));
								renderPassAttachmentInfo.renderPassType = Utils::EraseFromBack(renderPassType, '/');
								m_RenderPassAttachments.emplace(loadedValueName, renderPassAttachmentInfo);
							}
							else
							{
								m_TexturesByName[value.name] = TextureManager::GetInstance().Load(loadedValue);
							}
						}
					}
				}
			}
		}
	}
}

Material::~Material()
{
	m_Index = 0;
	m_BaseMaterial->m_MaterialsSize--;
}

std::shared_ptr<UniformWriter> Material::GetUniformWriter(const std::string& renderPassName) const
{
	return Utils::Find(renderPassName, m_UniformWriterByRenderPass);
}

std::shared_ptr<Texture> Material::GetTexture(const std::string& name) const
{
	return Utils::Find(name, m_TexturesByName);
}

void Material::SetTexture(const std::string& name, std::shared_ptr<Texture> texture)
{
	m_TexturesByName[name] = texture;
}
