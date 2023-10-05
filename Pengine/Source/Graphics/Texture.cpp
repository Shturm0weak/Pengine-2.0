#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "../Core/FileFormatNames.h"
#include "../Core/Logger.h"
#include "../SoftwareRenderer/SrTexture.h"
#include "../Vulkan/VulkanTexture.h"

using namespace Pengine;

std::shared_ptr<Texture> Texture::Create(CreateInfo textureCreateInfo)
{
	/*if (graphicsAPI == GraphicsAPI::Software)
	{
		return std::make_shared<SrTexture>(textureCreateInfo);
	}
	else*/
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanTexture>(textureCreateInfo);
	}
}

std::shared_ptr<Texture> Texture::Load(const std::string& filepath)
{
	stbi_set_flip_vertically_on_load(true);

	CreateInfo textureCreateInfo{};
	textureCreateInfo.name = filepath;
	textureCreateInfo.filepath = filepath;
	textureCreateInfo.aspectMask = AspectMask::COLOR;

	textureCreateInfo.format = Format::R8G8B8A8_SRGB;
	void* data = stbi_load(
		filepath.c_str(),
		&textureCreateInfo.size.x,
		&textureCreateInfo.size.y,
		&textureCreateInfo.channels,
		STBI_rgb_alpha);
	textureCreateInfo.channels = STBI_rgb_alpha;

	textureCreateInfo.data.resize(textureCreateInfo.size.x * textureCreateInfo.size.y * textureCreateInfo.channels);
	memcpy(textureCreateInfo.data.data(), data, textureCreateInfo.data.size());
	delete data;

	textureCreateInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(
		textureCreateInfo.size.x, textureCreateInfo.size.y)))) + 1;
	textureCreateInfo.usage = { Usage::SAMPLED, Usage::TRANSFER_DST, Usage::TRANSFER_SRC };

	if (textureCreateInfo.data.empty())
	{
		Logger::Error("Texture can't be loaded from the filepath " + filepath);

		return nullptr;
	}
	else
	{
		Logger::Log("Texture:" + textureCreateInfo.filepath + " has been loaded!", GREEN);
	}

	/*if (graphicsAPI == GraphicsAPI::Software)
	{
		return std::make_shared<SrTexture>(textureCreateInfo);
	}
	else*/
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanTexture>(textureCreateInfo);
	}
}

Texture::Texture(CreateInfo textureCreateInfo)
	: Asset(textureCreateInfo.name, textureCreateInfo.filepath)
{
	m_Size = textureCreateInfo.size;
	m_Channels = textureCreateInfo.channels;
	m_Data = textureCreateInfo.data;
	m_Format = textureCreateInfo.format;
	m_AspectMask = textureCreateInfo.aspectMask;
	m_MipLevels = textureCreateInfo.mipLevels;
}
