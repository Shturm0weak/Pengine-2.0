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
		const Texture::Meta meta = Serializer::DeserializeTextureMeta(filepath.string() + FileFormats::Meta());
		texture = Texture::Load(filepath, meta);
		if (texture)
		{
			std::lock_guard<std::mutex> lock(m_MutexTexture);
			m_TexturesByFilepath[filepath] = texture;

			return texture;
		}

		// Need to consider.
		//return GetWhite();
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

std::shared_ptr<Texture> TextureManager::GetTexture(const std::filesystem::path& filepath)
{
	std::lock_guard<std::mutex> lock(m_MutexTexture);
	if (const auto textureByFilepath = m_TexturesByFilepath.find(filepath);
		textureByFilepath != m_TexturesByFilepath.end())
	{
		return textureByFilepath->second;
	}

	return nullptr;
}

std::shared_ptr<Texture> TextureManager::GetWhite()
{
	return GetTexture("White");
}

std::shared_ptr<Texture> TextureManager::GetBlack()
{
	return GetTexture("Black");
}

void TextureManager::Delete(const std::filesystem::path& filepath)
{
	std::lock_guard<std::mutex> lock(m_MutexTexture);
	m_TexturesByFilepath.erase(filepath);
}

void TextureManager::ShutDown()
{
	std::lock_guard<std::mutex> lock(m_MutexTexture);
	m_TexturesByFilepath.clear();
}
