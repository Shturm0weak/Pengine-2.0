#include "Material.h"

#include "../Core/Profiler.h"
#include "../Core/Serializer.h"
#include "../Core/TextureManager.h"
#include "../Core/MaterialManager.h"
#include "../Core/AsyncAssetLoader.h"
#include "../Core/BindlessUniformWriter.h"
#include "../EventSystem/EventSystem.h"
#include "../EventSystem/NextFrameEvent.h"

using namespace Pengine;

std::shared_ptr<Material> Material::Create(const std::string& name, const std::filesystem::path& filepath,
	const CreateInfo& createInfo)
{
	PROFILER_SCOPE(__FUNCTION__);

	return std::make_shared<Material>(name, filepath, createInfo);
}

std::shared_ptr<Material> Material::Load(const std::filesystem::path& filepath)
{
	PROFILER_SCOPE(__FUNCTION__);

	return Create(Utils::GetFilename(filepath), filepath, Serializer::LoadMaterial(filepath));
}

void Material::Save(const std::shared_ptr<Material>& material, bool useLog)
{
	PROFILER_SCOPE(__FUNCTION__);

	Serializer::SerializeMaterial(material, useLog);
}

void Material::Reload(const std::shared_ptr<Material>& material, bool reloadBaseMaterial)
{
	PROFILER_SCOPE(__FUNCTION__);

	if (reloadBaseMaterial)
	{
		BaseMaterial::Reload(material->m_BaseMaterial);
	}

	auto callback = [material]()
	{
		material->m_UniformWriterByPass.clear();
		material->m_OptionsByName.clear();
		material->m_PipelineStates.clear();
		material->m_BuffersByName.clear();

		std::vector<std::shared_ptr<Texture>> textures;
		textures.reserve(material->m_BindlessTexturesByIndex.size());
		
		for (const auto& [index, texture] : material->m_BindlessTexturesByIndex)
		{
			textures.emplace_back(texture);
		}
		material->m_BindlessTexturesByIndex.clear();

		for (auto& texture : textures)
		{
			TextureManager::GetInstance().Delete(texture);
		}

		try
		{
			material->CreateResources(Serializer::LoadMaterial(material->GetFilepath()));
		}
		catch (const std::exception&)
		{

		}
	};

	std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, material.get());
	EventSystem::GetInstance().SendEvent(event);
}

std::shared_ptr<Material> Material::Clone(
	const std::string& name,
	const std::filesystem::path& filepath,
	const std::shared_ptr<Material>& material)
{
	PROFILER_SCOPE(__FUNCTION__);

	CreateInfo createInfo{};
	createInfo.baseMaterial = material->GetBaseMaterial()->GetFilepath();
	createInfo.optionsByName = material->GetOptionsByName();

	for (const auto& [passName, pipeline] : material->GetBaseMaterial()->GetPipelinesByPass())
	{
		std::optional<uint32_t> descriptorSetIndex = pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::MATERIAL, passName);
		if (!descriptorSetIndex)
		{
			continue;
		}
		for (const auto& binding : pipeline->GetUniformLayout(*descriptorSetIndex)->GetBindings())
		{
			if (binding.type == ShaderReflection::Type::COMBINED_IMAGE_SAMPLER)
			{
				createInfo.uniformInfos[passName].texturesByName[binding.name] = material->GetUniformWriter(passName)->GetTexture(binding.name).back()->GetFilepath().string();
			}
			else if (binding.type == ShaderReflection::Type::UNIFORM_BUFFER)
			{
				const std::shared_ptr<Buffer> buffer = material->GetBuffer(binding.name);
				void* data = buffer->GetData();

				auto& uniformBufferInfo = createInfo.uniformInfos[passName].uniformBuffersByName[binding.name];

				std::function<void(const ShaderReflection::ReflectVariable&, std::string)> copyValue = [
					data,
					&material,
					&copyValue,
					&uniformBufferInfo]
				(const ShaderReflection::ReflectVariable& value, std::string parentName)
				{
					parentName += value.name;

					if (value.type == ShaderReflection::ReflectVariable::Type::VEC2)
					{
						uniformBufferInfo.vec2ValuesByName.emplace(parentName, Utils::GetValue<glm::vec2>(data, value.offset));
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::VEC3)
					{
						uniformBufferInfo.vec3ValuesByName.emplace(parentName, Utils::GetValue<glm::vec3>(data, value.offset));
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::VEC4)
					{
						uniformBufferInfo.vec4ValuesByName.emplace(parentName, Utils::GetValue<glm::vec4>(data, value.offset));
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::FLOAT)
					{
						uniformBufferInfo.floatValuesByName.emplace(parentName, Utils::GetValue<float>(data, value.offset));
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::INT)
					{
						uniformBufferInfo.intValuesByName.emplace(parentName, Utils::GetValue<int>(data, value.offset));
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::STRUCT)
					{
						for (const auto& memberValue : value.variables)
						{
							copyValue(memberValue, parentName + ".");
						}
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::TEXTURE)
					{
						const int index = Utils::GetValue<int>(data, value.offset);
						uniformBufferInfo.texturesByName.emplace(parentName, material->GetBindlessTexture(index)->GetFilepath());
					}
				};

				for (const auto& value : binding.buffer->variables)
				{
					copyValue(value, "");
				}
			}
		}
	}

	return Create(name, filepath, createInfo);
}

Material::Material(
	const std::string& name,
	const std::filesystem::path& filepath,
	const CreateInfo& createInfo)
	: Asset(name, filepath)
{
	CreateResources(createInfo);
}

Material::~Material()
{
	std::vector<std::shared_ptr<Texture>> textures;
	textures.reserve(m_BindlessTexturesByIndex.size());
	
	for (const auto& [index, texture] : m_BindlessTexturesByIndex)
	{
		textures.emplace_back(texture);
	}
	m_BindlessTexturesByIndex.clear();

	for (auto& texture : textures)
	{
		TextureManager::GetInstance().Delete(texture);
	}

	MaterialManager::GetInstance().DeleteBaseMaterial(m_BaseMaterial);
}

std::shared_ptr<UniformWriter> Material::GetUniformWriter(const std::string& passName) const
{
	return Utils::Find(passName, m_UniformWriterByPass);
}

std::shared_ptr<Buffer> Material::GetBuffer(const std::string& name) const
{
	return Utils::Find(name, m_BuffersByName);
}

bool Material::IsPipelineEnabled(const std::string& passName) const
{
	auto pipelineState = m_PipelineStates.find(passName);
	if (pipelineState != m_PipelineStates.end())
	{
		return pipelineState->second;
	}

	return true;
}

void Material::SetOption(const std::string& name, bool isEnabled)
{
	auto foundOption = m_OptionsByName.find(name);
	if (foundOption == m_OptionsByName.end())
	{
		return;
	}

	foundOption->second.m_IsEnabled = isEnabled;

	for (const std::string& active : foundOption->second.m_Active)
	{
		m_PipelineStates[active] = isEnabled;
	}

	for (const std::string& inactive : foundOption->second.m_Inactive)
	{
		m_PipelineStates[inactive] = !isEnabled;
	}
}

std::shared_ptr<Texture> Pengine::Material::GetBindlessTexture(const int index) const
{
	auto bindlessTextureByIndex = m_BindlessTexturesByIndex.find(index);
	if (bindlessTextureByIndex != m_BindlessTexturesByIndex.end())
	{
		return bindlessTextureByIndex->second;
	}

	return nullptr;
}

int Pengine::Material::BindBindlessTexture(const std::shared_ptr<Texture>& texture)
{
    const int index = BindlessUniformWriter::GetInstance().BindTexture(texture);
	m_BindlessTexturesByIndex[index] = texture;
	return index;
}

void Pengine::Material::UnBindBindlessTexture(const std::shared_ptr<Texture>& texture)
{
	m_BindlessTexturesByIndex.erase(texture->GetBindlessIndex());
	BindlessUniformWriter::GetInstance().UnBindTexture(texture);
}

void Material::CreateResources(const CreateInfo &createInfo)
{
	m_BaseMaterial = AsyncAssetLoader::GetInstance().SyncLoadBaseMaterial(createInfo.baseMaterial);
	m_OptionsByName = createInfo.optionsByName;
	for (const auto& [name, option] : m_OptionsByName)
	{
		SetOption(name, option.m_IsEnabled);
	}

	for (const auto& [passName, pipeline] : m_BaseMaterial->GetPipelinesByPass())
	{
		auto baseMaterialIndex = pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::MATERIAL, passName);
		if (baseMaterialIndex)
		{
			const std::shared_ptr<UniformLayout> uniformLayout = pipeline->GetUniformLayout(*baseMaterialIndex);
			const std::shared_ptr<UniformWriter> uniformWriter = UniformWriter::Create(uniformLayout);
			m_UniformWriterByPass[passName] = uniformWriter;

			for (const auto& binding : uniformLayout->GetBindings())
			{
				if (binding.buffer)
				{
					const std::shared_ptr<Buffer> buffer = Buffer::Create(
						binding.buffer->size,
						1,
						Buffer::Usage::UNIFORM_BUFFER,
						MemoryType::CPU);
					m_BuffersByName[binding.buffer->name] = buffer;
					uniformWriter->WriteBuffer(binding.buffer->name, buffer);
					uniformWriter->Flush();
				}
			}
		}
	}

	for (const auto& [passName, uniformInfo] : createInfo.uniformInfos)
	{
		if (uniformInfo.texturesByName.empty() && uniformInfo.uniformBuffersByName.empty())
		{
			continue;
		}

		const std::shared_ptr<UniformWriter> uniformWriter = GetUniformWriter(passName);
		if (!uniformWriter)
		{
			continue;
		}

		for (const auto& [name, filepath] : uniformInfo.texturesByName)
		{
			std::shared_ptr<Texture> texture = TextureManager::GetInstance().Load(filepath);
			uniformWriter->WriteTexture(name, texture);
		}
		uniformWriter->Flush();

		for (const auto& [uniformBufferName, bufferInfo] : uniformInfo.uniformBuffersByName)
		{
			for (auto const& [loadedValueName, loadedValue] : bufferInfo.floatValuesByName)
			{
				WriteToBuffer(uniformBufferName, loadedValueName, loadedValue);
			}

			for (auto const& [loadedValueName, loadedValue] : bufferInfo.intValuesByName)
			{
				WriteToBuffer(uniformBufferName, loadedValueName, loadedValue);
			}

			for (auto const& [loadedValueName, loadedValue] : bufferInfo.vec4ValuesByName)
			{
				WriteToBuffer(uniformBufferName, loadedValueName, loadedValue);
			}

			for (auto const& [loadedValueName, loadedValue] : bufferInfo.vec3ValuesByName)
			{
				WriteToBuffer(uniformBufferName, loadedValueName, loadedValue);
			}

			for (auto const& [loadedValueName, loadedValue] : bufferInfo.vec2ValuesByName)
			{
				WriteToBuffer(uniformBufferName, loadedValueName, loadedValue);
			}

			for (const auto& [name, filepath] : bufferInfo.texturesByName)
			{
				std::shared_ptr<Texture> texture = TextureManager::GetInstance().Load(filepath);
				const int index = BindBindlessTexture(texture);
				WriteToBuffer(uniformBufferName, name, index);
			}
		}
	}
}
