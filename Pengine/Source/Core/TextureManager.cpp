#include "TextureManager.h"

#include "FileFormatNames.h"

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
	m_TexturesByFilepath[createInfo.filepath] = texture;
	return texture;
}

std::shared_ptr<Texture> TextureManager::Load(const std::string& filepath)
{
	if (std::shared_ptr<Texture> texture = GetTexture(filepath))
	{
		return texture;
	}
	else
	{
		texture = Texture::Load(filepath);
		if (texture)
		{
			m_TexturesByFilepath[filepath] = texture;
			return texture;
		}
		else
		{
			return GetWhite();
		}
	}
}

std::vector<std::shared_ptr<Texture>> TextureManager::LoadFromFolder(const std::string& directory)
{
	std::vector<std::shared_ptr<Texture>> textures;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
	{
		if (!entry.is_directory())
		{
			std::string filepath = entry.path().string();
			filepath = Utils::Replace(filepath, '\\', '/');

			if (FileFormats::IsTexture(Utils::GetFileFormat(filepath)))
			{
				textures.emplace_back(Load(filepath));
			}
		}
	}

	return textures;
}

std::shared_ptr<Texture> TextureManager::GetTexture(const std::string& filepath) const
{
	auto textureByFilepath = m_TexturesByFilepath.find(filepath);
	if (textureByFilepath != m_TexturesByFilepath.end())
	{
		return textureByFilepath->second;
	}

	return nullptr;
}

std::shared_ptr<Texture> TextureManager::GetWhite() const
{
	return GetTexture("White");
}

void TextureManager::ShutDown()
{
	m_TexturesByFilepath.clear();
}
