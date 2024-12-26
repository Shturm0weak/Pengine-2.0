#pragma once

#include "Core.h"

#include "../Graphics/Texture.h"

#include <mutex>

namespace Pengine
{

	class PENGINE_API TextureManager
	{
	public:
		static TextureManager& GetInstance();

		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;

		std::shared_ptr<Texture> Create(const Texture::CreateInfo& createInfo);

		std::shared_ptr<Texture> Load(const std::filesystem::path& filepath);

		std::vector<std::shared_ptr<Texture>> LoadFromFolder(const std::filesystem::path& directory);

		std::shared_ptr<Texture> GetTexture(const std::filesystem::path& filepath) const;

		const std::unordered_map<std::filesystem::path, std::shared_ptr<Texture>, path_hash>& GetTextures() const { return m_TexturesByFilepath; }

		std::shared_ptr<Texture> GetWhite() const;

		std::shared_ptr<Texture> GetBlack() const;

		void Delete(const std::filesystem::path& filepath);

		void ShutDown();

	private:
		TextureManager() = default;
		~TextureManager() = default;

		std::unordered_map<std::filesystem::path, std::shared_ptr<Texture>, path_hash> m_TexturesByFilepath;

		std::mutex m_Mutex;
	};

}