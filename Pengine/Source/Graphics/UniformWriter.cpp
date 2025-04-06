#include "UniformWriter.h"

#include "../Core/Logger.h"
#include "../Core/TextureManager.h"
#include "../Utils/Utils.h"
#include "../Vulkan/VulkanUniformWriter.h"

using namespace Pengine;

std::shared_ptr<UniformWriter> UniformWriter::Create(
	std::shared_ptr<UniformLayout> uniformLayout,
	bool isMultiBuffered)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanUniformWriter>(uniformLayout, isMultiBuffered);
	}

	FATAL_ERROR("Failed to create the uniform writer, no graphics API implementation");
	return nullptr;
}

UniformWriter::UniformWriter(
	std::shared_ptr<UniformLayout> uniformLayout,
	bool isMultiBuffered)
	: m_UniformLayout(uniformLayout)
	, m_IsMultiBuffered(isMultiBuffered)
{
	m_Writes.resize(m_IsMultiBuffered ? Vk::swapChainImageCount : 1);
}

UniformWriter::~UniformWriter()
{
	for (auto& [name, textures] : m_TexturesByName)
	{
		for (auto& texture : textures)
		{
			TextureManager::GetInstance().Delete(texture);
		}
	}
}

void UniformWriter::WriteBuffer(uint32_t location, const std::shared_ptr<Buffer>& buffer, size_t size, size_t offset)
{
	const auto binding = m_UniformLayout->GetBindingByLocation(location);
	if (!binding)
	{
		FATAL_ERROR("Layout does not contain specified binding!");
	}

	BufferWrite write{};
	write.buffers = { buffer };
	write.offset = offset;
	write.size = size;
	write.binding = *binding;

	for (size_t i = 0; i < m_Writes.size(); i++)
	{
		m_Writes[i].m_BufferWritesByLocation[location] = write;
	}
}

void UniformWriter::WriteTexture(uint32_t location, const std::shared_ptr<Texture>& texture)
{
	const auto binding = m_UniformLayout->GetBindingByLocation(location);
	if (!binding)
	{
		FATAL_ERROR("Layout does not contain specified binding!");
	}

	TextureWrite write{};
	write.textures = { texture };
	write.binding = *binding;

	for (size_t i = 0; i < m_Writes.size(); i++)
	{
		m_Writes[i].m_TextureWritesByLocation[location] = write;
	}
}

void UniformWriter::WriteTextures(uint32_t location, const std::vector<std::shared_ptr<Texture>>& textures)
{
	const auto binding = m_UniformLayout->GetBindingByLocation(location);
	if (!binding)
	{
		FATAL_ERROR("Layout does not contain specified binding!");
	}

	TextureWrite write{};
	write.textures = textures;
	write.binding = *binding;

	for (size_t i = 0; i < m_Writes.size(); i++)
	{
		m_Writes[i].m_TextureWritesByLocation[location] = write;
	}
}

void UniformWriter::WriteBuffer(const std::string& name, const std::shared_ptr<Buffer>& buffer, size_t size, size_t offset)
{
	const uint32_t location = m_UniformLayout->GetBindingLocationByName(name);
	m_BufferNameByLocation[location] = name;
	m_BuffersByName[name] = { buffer };

	WriteBuffer(location, buffer, size, offset);
}

void UniformWriter::WriteTexture(const std::string& name, const std::shared_ptr<Texture>& texture)
{
	const uint32_t location = m_UniformLayout->GetBindingLocationByName(name);
	m_TextureNameByLocation[location] = name;
	m_TexturesByName[name] = { texture };

	WriteTexture(location, texture);
}

void UniformWriter::WriteTextures(const std::string& name, const std::vector<std::shared_ptr<Texture>>& textures)
{
	const uint32_t location = m_UniformLayout->GetBindingLocationByName(name);
	m_TextureNameByLocation[location] = name;
	m_TexturesByName[name] = textures;

	WriteTextures(location, textures);
}

std::vector<std::shared_ptr<Buffer>> UniformWriter::GetBuffer(const std::string& name)
{
	return Utils::Find(name, m_BuffersByName);
}

std::vector<std::shared_ptr<Texture>> UniformWriter::GetTexture(const std::string& name)
{
	return Utils::Find(name, m_TexturesByName);
}
