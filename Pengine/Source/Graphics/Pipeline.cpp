#include "Pipeline.h"

#include "../Core/Logger.h"
#include "../Utils/Utils.h"
#include "../Vulkan/VulkanPipeline.h"

#include <fstream>

using namespace Pengine;

std::shared_ptr<Pipeline> Pipeline::Create(const CreateInfo& pipelineCreateInfo)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanPipeline>(pipelineCreateInfo);
	}

	FATAL_ERROR("Failed to create the pipeline, no graphics API implementation");
	return nullptr;
}

Pipeline::Pipeline(const CreateInfo& pipelineCreateInfo) : createInfo(pipelineCreateInfo)
{
	if (!pipelineCreateInfo.uniformBindings.empty())
	{
		const std::shared_ptr<UniformLayout> layout = UniformLayout::Create(pipelineCreateInfo.uniformBindings);
		m_UniformWriter = UniformWriter::Create(layout);

		for (const auto& [location, binding] : layout->GetBindingsByLocation())
		{
			if (binding.type == UniformLayout::Type::BUFFER)
			{
				size_t bufferSize = 0;
				for (const auto& value : binding.values)
				{
					bufferSize += Utils::StringTypeToSize(value.type);
				}

				// Performance issue can appear due to Buffer::MemoryType::CPU.
				const auto buffer = Buffer::Create(
					bufferSize,
					1,
					Buffer::Usage::UNIFORM_BUFFER,
					Buffer::MemoryType::CPU);

				m_BuffersByName[binding.name] = buffer;
				m_UniformWriter->WriteBuffer(location, buffer);
			}
		}
		m_UniformWriter->Flush();
	}

	if (!pipelineCreateInfo.childUniformBindings.empty())
	{
		m_ChildUniformLayout = UniformLayout::Create(pipelineCreateInfo.childUniformBindings);
		for (const auto& [location, binding] : m_ChildUniformLayout->GetBindingsByLocation())
		{
			if (binding.type == UniformLayout::Type::BUFFER)
			{
				size_t bufferSize = 0;
				for (const auto& value : binding.values)
				{
					bufferSize += Utils::StringTypeToSize(value.type);
				}

				// Performance issue can appear due to Buffer::MemoryType::CPU.
				const auto buffer = Buffer::Create(
					bufferSize,
					MAX_MATERIALS,
					Buffer::Usage::UNIFORM_BUFFER,
					Buffer::MemoryType::CPU);

				m_BuffersByName[binding.name] = buffer;
			}
		}
	}
}

std::shared_ptr<Buffer> Pipeline::GetBuffer(const std::string& name) const
{
	return Utils::Find(name, m_BuffersByName);
}

std::string Pipeline::ReadFile(const std::string& filepath)
{
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		FATAL_ERROR("Failed to open file: " + filepath);
	}

	const size_t fileSize = file.tellg();

	std::string buffer;
	buffer.resize(fileSize);

	file.seekg(0);
	file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
	file.close();

	return buffer;
}