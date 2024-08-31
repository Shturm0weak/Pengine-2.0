#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "../Core/Logger.h"
#include "../Vulkan/VulkanTexture.h"
#include "../Utils/Utils.h"

using namespace Pengine;

std::shared_ptr<Texture> Texture::Create(const CreateInfo& textureCreateInfo)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanTexture>(textureCreateInfo);
	}

	FATAL_ERROR("Failed to create the texture, no graphics API implementation");
	return nullptr;
}

std::shared_ptr<Texture> Texture::Load(const std::filesystem::path& filepath)
{
	stbi_set_flip_vertically_on_load(true);

	CreateInfo textureCreateInfo{};
	textureCreateInfo.name = Utils::GetFilename(filepath);
	textureCreateInfo.filepath = filepath;
	textureCreateInfo.aspectMask = AspectMask::COLOR;

	textureCreateInfo.format = Format::R8G8B8A8_SRGB;
	void* data = stbi_load(
		filepath.string().c_str(),
		&textureCreateInfo.size.x,
		&textureCreateInfo.size.y,
		&textureCreateInfo.channels,
		STBI_rgb_alpha);
	textureCreateInfo.channels = STBI_rgb_alpha;
	textureCreateInfo.instanceSize = sizeof(uint8_t);

	const size_t size = textureCreateInfo.size.x * textureCreateInfo.size.y * textureCreateInfo.channels * textureCreateInfo.instanceSize;
	textureCreateInfo.data = data;

	textureCreateInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(
		textureCreateInfo.size.x, textureCreateInfo.size.y)))) + 1;
	textureCreateInfo.usage = { Usage::SAMPLED, Usage::TRANSFER_DST, Usage::TRANSFER_SRC };

	if (!textureCreateInfo.data)
	{
		Logger::Error("Texture can't be loaded from the filepath " + filepath.string());
		return nullptr;
	}

	Logger::Log("Texture:" + textureCreateInfo.filepath.string() + " has been loaded!", GREEN);

	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanTexture>(textureCreateInfo);
	}

	stbi_image_free(data);

	FATAL_ERROR("Failed to create the renderer, no graphics API implementation");
	return nullptr;
}

Texture::Texture(const CreateInfo& textureCreateInfo)
	: Asset(textureCreateInfo.name, textureCreateInfo.filepath)
{
	m_Size = textureCreateInfo.size;
	m_Channels = textureCreateInfo.channels;
	m_Format = textureCreateInfo.format;
	m_AspectMask = textureCreateInfo.aspectMask;
	m_MipLevels = textureCreateInfo.mipLevels;
	m_LayerCount = textureCreateInfo.isCubeMap ? 6 : 1;
	m_InstanceSize = textureCreateInfo.instanceSize;
}
