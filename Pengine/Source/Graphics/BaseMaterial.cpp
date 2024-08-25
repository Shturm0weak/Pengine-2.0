#include "BaseMaterial.h"

#include "../Core/Logger.h"
#include "../Core/Serializer.h"
#include "../Core/TextureManager.h"

#include <algorithm>

using namespace Pengine;

std::shared_ptr<BaseMaterial> BaseMaterial::Create(
	const std::string& name,
	const std::filesystem::path& filepath,
	const CreateInfo& createInfo)
{
	return std::make_shared<BaseMaterial>(name, filepath, createInfo);
}

std::shared_ptr<BaseMaterial> BaseMaterial::Load(const std::filesystem::path& filepath)
{
	const CreateInfo createInfo = Serializer::LoadBaseMaterial(filepath);

	return Create(Utils::GetFilename(filepath), filepath, createInfo);
}

BaseMaterial::BaseMaterial(
	const std::string& name,
	const std::filesystem::path& filepath,
	const CreateInfo& createInfo)
	: Asset(name, filepath)
{
	for (const Pipeline::CreateInfo& pipelineCreateInfo : createInfo.pipelineCreateInfos)
	{
		const std::string renderPassName = pipelineCreateInfo.renderPass->GetType();

		const std::shared_ptr<Pipeline> pipeline = Pipeline::Create(pipelineCreateInfo);

		auto baseMaterialIndex = pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::BASE_MATERIAL, renderPassName);
		if (baseMaterialIndex)
		{
			const std::shared_ptr<UniformLayout> uniformLayout = pipeline->GetUniformLayout(*baseMaterialIndex);
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

		m_PipelinesByRenderPass[renderPassName] = pipeline;

		if (pipelineCreateInfo.uniformInfo.texturesByName.empty() && pipelineCreateInfo.uniformInfo.uniformBuffersByName.empty())
		{
			continue;
		}

		const std::shared_ptr<UniformWriter> uniformWriter = GetUniformWriter(renderPassName);
		for (const auto& [name, filepath] : pipelineCreateInfo.uniformInfo.texturesByName)
		{
			std::shared_ptr<Texture> texture = TextureManager::GetInstance().Load(filepath);
			uniformWriter->WriteTexture(name, texture);
		}
		uniformWriter->Flush();

		for (const auto& [uniformBufferName, bufferInfo] : pipelineCreateInfo.uniformInfo.uniformBuffersByName)
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

BaseMaterial::~BaseMaterial()
{
	m_PipelinesByRenderPass.clear();
}

std::shared_ptr<Pipeline> BaseMaterial::GetPipeline(const std::string& renderPassName) const
{
	if (const auto pipelineByRenderPass = m_PipelinesByRenderPass.find(renderPassName);
		pipelineByRenderPass != m_PipelinesByRenderPass.end())
	{
		return pipelineByRenderPass->second;
	}
	
	return nullptr;
}

std::shared_ptr<UniformWriter> BaseMaterial::GetUniformWriter(const std::string& renderPassName) const
{
	return Utils::Find(renderPassName, m_UniformWriterByRenderPass);
}

std::shared_ptr<Buffer> BaseMaterial::GetBuffer(const std::string& name) const
{
	return Utils::Find(name, m_BuffersByName);
}

bool BaseMaterial::GetUniformDetails(
	const std::string& uniformBufferName,
	const std::string& valueName,
	uint32_t& size,
	uint32_t& offset) const
{
	std::function<bool(
		const std::vector<ShaderReflection::ReflectVariable>&,
		std::string,
		const uint32_t,
		uint32_t&,
		uint32_t&)> findVariable;

	findVariable = [&findVariable](
		const std::vector<ShaderReflection::ReflectVariable>& variables,
		std::string valueName,
		const uint32_t parentOffset,
		uint32_t& size,
		uint32_t& offset)
		{
			std::string searchingName;

			std::optional<int> arrayIndex;

			const size_t squareBracketOpenOffset = valueName.find_first_of('[');
			if (squareBracketOpenOffset != std::string::npos)
			{
				const size_t squareBracketCloseOffset = valueName.find_first_of(']');
				if (squareBracketCloseOffset == std::string::npos)
				{
					Logger::Warning("Failed to write to buffer: " + valueName + ", because missing closing ]");
					return false;
				}

				const std::string arrayIndexString = valueName.substr(squareBracketOpenOffset + 1, squareBracketCloseOffset - squareBracketOpenOffset - 1);
				arrayIndex = std::stoi(arrayIndexString);
			}

			const size_t dotOffset = valueName.find_first_of('.');
			if (dotOffset != std::string::npos)
			{
				if (squareBracketOpenOffset != std::string::npos)
				{
					searchingName = valueName.substr(0, squareBracketOpenOffset);

				}
				else
				{
					searchingName = valueName.substr(0, dotOffset);
				}

				valueName = valueName.substr(dotOffset + 1, valueName.size() - dotOffset - 1);
			}
			else
			{
				searchingName = valueName;
			}

			auto variable = std::find_if(
				variables.begin(),
				variables.end(),
				[searchingName](const ShaderReflection::ReflectVariable& variable)
				{ 
					return variable.name == searchingName;
				}
			);
			
			if (variable == variables.end())
			{
				return false;
			}

			uint32_t currentParentOffset = parentOffset;
			if (arrayIndex)
			{
				if (*arrayIndex > variable->count)
				{
					Logger::Warning("Failed to write to buffer: " + valueName + ", because the array index is greater than the count " + 
						std::to_string(variable->count) +  " of the array");
					return false;
				}

				const uint32_t instanceSize = variable->size / variable->count;
				currentParentOffset += instanceSize * *arrayIndex;
			}

			if (valueName == searchingName)
			{
				offset = variable->offset + parentOffset;
				size = variable->size;
				return true;
			}
			else if (!variable->variables.empty())
			{
				return findVariable(
					variable->variables,
					valueName,
					currentParentOffset + variable->offset,
					size,
					offset);
			}

			return false;
		};

	for (const auto& [renderPassName, pipeline] : m_PipelinesByRenderPass)
	{
		for (const auto& [set, layout] : pipeline->GetUniformLayouts())
		{
			for (const auto& binding : layout->GetBindings())
			{
				if (binding.buffer && binding.name == uniformBufferName)
				{
					const bool found = findVariable(binding.buffer->variables, valueName, 0, size, offset);

					if (found)
					{
						return true;
					}
				}
			}
		}
	}

	size = 0;
	offset = 0;

	return false;
}

std::optional<ShaderReflection::ReflectVariable> BaseMaterial::GetUniformValue(
	const std::string& uniformBufferName,
	const std::string& valueName)
{
	std::function<std::optional<ShaderReflection::ReflectVariable>(
		const std::vector<ShaderReflection::ReflectVariable>&,
		std::string)> findVariable;

	findVariable = [&findVariable](
		const std::vector<ShaderReflection::ReflectVariable>& variables,
		std::string valueName) -> std::optional<ShaderReflection::ReflectVariable>
	{
		std::string searchingName;

		const size_t dotOffset = valueName.find_first_of('.');
		if (dotOffset != std::string::npos)
		{
			searchingName = valueName.substr(0, dotOffset);
			valueName = valueName.substr(dotOffset + 1, valueName.size() - dotOffset - 1);
		}
		else
		{
			searchingName = valueName;
		}

		auto variable = std::find_if(
			variables.begin(),
			variables.end(),
			[searchingName](const ShaderReflection::ReflectVariable& variable)
			{
				return variable.name == searchingName;
			}
		);

		if (variable == variables.end())
		{
			return std::nullopt;
		}

		if (!variable->variables.empty())
		{
			return findVariable(
				variable->variables,
				valueName);
		}

		return *variable;
	};

	for (const auto& [renderPassName, pipeline] : m_PipelinesByRenderPass)
	{
		for (const auto& [set, layout] : pipeline->GetUniformLayouts())
		{
			for (const auto& binding : layout->GetBindings())
			{
				if (binding.buffer && binding.name == uniformBufferName)
				{
					return findVariable(binding.buffer->variables, valueName);
				}
			}
		}
	}

	return std::nullopt;
}
