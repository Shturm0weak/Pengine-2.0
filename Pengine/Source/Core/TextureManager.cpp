#include "TextureManager.h"

#include "FileFormatNames.h"
#include "Serializer.h"

#include "../Utils/Utils.h"

#include <filesystem>

using namespace Pengine;

TextureManager& TextureManager::GetInstance()
{
	static TextureManager textureManager;
	return textureManager;
}

std::shared_ptr<Texture> TextureManager::Create(const Texture::CreateInfo& createInfo)
{
	std::shared_ptr<Texture> texture = Texture::Create(createInfo);

	std::lock_guard<std::mutex> lock(m_MutexTexture);
	m_TexturesByFilepath[createInfo.filepath] = texture;

	return texture;
}

std::shared_ptr<Texture> TextureManager::Load(const std::filesystem::path& filepath)
{
	if (std::shared_ptr<Texture> texture = GetTexture(filepath))
	{
		return texture;
	}
	else
	{
		auto meta = Serializer::DeserializeTextureMeta(filepath.string() + FileFormats::Meta());
		texture = Texture::Load(filepath, *meta);
		if (texture)
		{
			std::lock_guard<std::mutex> lock(m_MutexTexture);
			m_TexturesByFilepath[filepath] = texture;

			return texture;
		}

		return GetPink();
	}
}

std::vector<std::shared_ptr<Texture>> TextureManager::LoadFromFolder(const std::filesystem::path& directory)
{
	std::vector<std::shared_ptr<Texture>> textures;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
	{
		if (!entry.is_directory())
		{
			if (FileFormats::IsTexture(Utils::GetFileFormat(entry.path())))
			{
				textures.emplace_back(Load(entry.path()));
			}
		}
	}

	return textures;
}

std::shared_ptr<Texture> TextureManager::GetTexture(const std::filesystem::path& filepath) const
{
	std::lock_guard<std::mutex> lock(m_MutexTexture);
	if (const auto textureByFilepath = m_TexturesByFilepath.find(filepath);
		textureByFilepath != m_TexturesByFilepath.end())
	{
		return textureByFilepath->second;
	}

	return nullptr;
}

std::shared_ptr<Texture> TextureManager::GetWhite() const
{
	return m_White;
}

std::shared_ptr<Texture> TextureManager::GetWhiteLayered() const
{
	return m_WhiteLayered;
}

std::shared_ptr<Texture> TextureManager::GetBlack() const
{
	return m_Black;
}

std::shared_ptr<Texture> TextureManager::GetPink() const
{
	return m_Pink;
}

void TextureManager::CreateDefaultResources()
{
	Texture::CreateInfo whiteTextureCreateInfo{};
	whiteTextureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	whiteTextureCreateInfo.channels = 4;
	whiteTextureCreateInfo.filepath = "White";
	whiteTextureCreateInfo.name = "White";
	whiteTextureCreateInfo.format = Format::R8G8B8A8_SRGB;
	whiteTextureCreateInfo.size = { 1, 1 };
	whiteTextureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_DST };
	std::vector<uint8_t> whitePixels = {
		255,
		255,
		255,
		255
	};
	whiteTextureCreateInfo.data = whitePixels.data();
	m_White = TextureManager::GetInstance().Create(whiteTextureCreateInfo);

	Texture::CreateInfo pinkTextureCreateInfo{};
	pinkTextureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	pinkTextureCreateInfo.channels = 4;
	pinkTextureCreateInfo.filepath = "Pink";
	pinkTextureCreateInfo.name = "Pink";
	pinkTextureCreateInfo.format = Format::R8G8B8A8_SRGB;
	pinkTextureCreateInfo.size = { 1, 1 };
	pinkTextureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_DST };
	std::vector<uint8_t> pinkPixels = {
		255,
		0,
		255,
		255
	};
	pinkTextureCreateInfo.data = pinkPixels.data();
	m_Pink = TextureManager::GetInstance().Create(pinkTextureCreateInfo);

	Texture::CreateInfo blackTextureCreateInfo{};
	blackTextureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	blackTextureCreateInfo.channels = 4;
	blackTextureCreateInfo.filepath = "Black";
	blackTextureCreateInfo.name = "Black";
	blackTextureCreateInfo.format = Format::R8G8B8A8_SRGB;
	blackTextureCreateInfo.size = { 1, 1 };
	blackTextureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_DST };
	std::vector<uint8_t> blackPixels = {
		0,
		0,
		0,
		0
	};
	blackTextureCreateInfo.data = blackPixels.data();
	m_Black = TextureManager::GetInstance().Create(blackTextureCreateInfo);

	Texture::CreateInfo whiteLayeredTextureCreateInfo;
	whiteLayeredTextureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	whiteLayeredTextureCreateInfo.channels = 4;
	whiteLayeredTextureCreateInfo.filepath = "WhiteLayered";
	whiteLayeredTextureCreateInfo.name = "WhiteLayered";
	whiteLayeredTextureCreateInfo.format = Format::R8G8B8A8_SRGB;
	whiteLayeredTextureCreateInfo.size = { 1, 1 };
	whiteLayeredTextureCreateInfo.layerCount = 2;;
	whiteLayeredTextureCreateInfo.data = nullptr;
	whiteLayeredTextureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::COLOR_ATTACHMENT, Texture::Usage::TRANSFER_DST };
	m_WhiteLayered = TextureManager::GetInstance().Create(whiteLayeredTextureCreateInfo);
}

void TextureManager::Delete(const std::filesystem::path& filepath)
{
	std::lock_guard<std::mutex> lock(m_MutexTexture);
	m_TexturesByFilepath.erase(filepath);
}

void TextureManager::Delete(std::shared_ptr<Texture>& texture)
{
	if (texture.use_count() == 2)
	{
		Delete(texture->GetFilepath());
	}

	texture = nullptr;
}

void TextureManager::ShutDown()
{
	std::lock_guard<std::mutex> lock(m_MutexTexture);
	m_TexturesByFilepath.clear();

	m_WhiteLayered = nullptr;
	m_White = nullptr;
	m_Black = nullptr;
	m_Pink = nullptr;
}
