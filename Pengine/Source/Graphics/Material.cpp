#include "Material.h"

#include "../Core/Serializer.h"
#include "../Core/TextureManager.h"

using namespace Pengine;

std::shared_ptr<Material> Material::Create(const std::string& name, const std::filesystem::path& filepath,
	const CreateInfo& createInfo)
{
	return std::make_shared<Material>(name, filepath, createInfo);
}

std::shared_ptr<Material> Material::Load(const std::filesystem::path& filepath)
{
	return Create(Utils::GetFilename(filepath), filepath, Serializer::LoadMaterial(filepath));
}

void Material::Save(const std::shared_ptr<Material>& material)
{
	Serializer::SerializeMaterial(material);
}

std::shared_ptr<Material> Material::Clone(
	const std::string& name,
	const std::filesystem::path& filepath,
	const std::shared_ptr<Material>& material)
{
	CreateInfo createInfo{};
	createInfo.baseMaterial = material->GetBaseMaterial();

	for (const auto& [renderPassName, pipeline] : createInfo.baseMaterial->GetPipelinesByRenderPass())
	{
		for (const auto& binding : pipeline->GetUniformLayout(pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::MATERIAL).value_or(-1))->GetBindings())
		{
			if (binding.type == ShaderReflection::Type::COMBINED_IMAGE_SAMPLER)
			{
				createInfo.uniformInfos[renderPassName].texturesByName[binding.name] = material->GetUniformWriter(renderPassName)->GetTexture(binding.name)->GetFilepath().string();
			}
			else if (binding.type == ShaderReflection::Type::UNIFORM_BUFFER)
			{
				const std::shared_ptr<Buffer> buffer = material->GetBuffer(binding.name);
				void* data = buffer->GetData();

				auto& uniformBufferInfo = createInfo.uniformInfos[renderPassName].uniformBuffersByName[binding.name];

				for (const auto& value : binding.buffer->variables)
				{
					if (value.type == ShaderReflection::ReflectVariable::Type::VEC2)
					{
						uniformBufferInfo.vec2ValuesByName.emplace(value.name, Utils::GetValue<glm::vec2>(data, value.offset));
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::VEC3)
					{
						uniformBufferInfo.vec3ValuesByName.emplace(value.name, Utils::GetValue<glm::vec3>(data, value.offset));
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::VEC4)
					{
						uniformBufferInfo.vec4ValuesByName.emplace(value.name, Utils::GetValue<glm::vec4>(data, value.offset));
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::FLOAT)
					{
						uniformBufferInfo.floatValuesByName.emplace(value.name, Utils::GetValue<float>(data, value.offset));
					}
					else if (value.type == ShaderReflection::ReflectVariable::Type::INT)
					{
						uniformBufferInfo.intValuesByName.emplace(value.name, Utils::GetValue<int>(data, value.offset));
					}
				}
			}
		}
	}

	return Create(name, filepath, createInfo);
}

Material::Material(const std::string& name, const std::filesystem::path& filepath,
	const CreateInfo& createInfo)
	: Asset(name, filepath)
	, m_BaseMaterial(createInfo.baseMaterial)
{
	for (const auto& [renderPassName, pipeline] : m_BaseMaterial->GetPipelinesByRenderPass())
	{
		const Pipeline::CreateInfo& pipelineCreateInfo = pipeline->GetCreateInfo();
		auto baseMaterialIndex = pipelineCreateInfo.descriptorSetIndicesByType.find(Pipeline::DescriptorSetIndexType::MATERIAL);
		if (baseMaterialIndex != pipelineCreateInfo.descriptorSetIndicesByType.end())
		{
			const std::shared_ptr<UniformLayout> uniformLayout = pipeline->GetUniformLayout(baseMaterialIndex->second);
			const std::shared_ptr<UniformWriter> uniformWriter = UniformWriter::Create(uniformLayout);
			m_UniformWriterByRenderPass[renderPassName] = uniformWriter;

			for (const auto& binding : uniformLayout->GetBindings())
			{
				if (binding.buffer)
				{
					const std::shared_ptr<Buffer> buffer = Buffer::Create(
						binding.buffer->size,
						1,
						Buffer::Usage::UNIFORM_BUFFER,
						Buffer::MemoryType::CPU);
					m_BuffersByName[binding.buffer->name] = buffer;
					uniformWriter->WriteBuffer(binding.buffer->name, buffer);
					uniformWriter->Flush();
				}
			}
		}
	}

	for (const auto& [renderPassName, uniformInfo] : createInfo.uniformInfos)
	{
		if (uniformInfo.texturesByName.empty() && uniformInfo.uniformBuffersByName.empty())
		{
			continue;
		}

		const std::shared_ptr<UniformWriter> uniformWriter = GetUniformWriter(renderPassName);
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
		}
	}
}

std::shared_ptr<UniformWriter> Material::GetUniformWriter(const std::string& renderPassName) const
{
	return Utils::Find(renderPassName, m_UniformWriterByRenderPass);
}

std::shared_ptr<Buffer> Material::GetBuffer(const std::string& name) const
{
	return Utils::Find(name, m_BuffersByName);
}
