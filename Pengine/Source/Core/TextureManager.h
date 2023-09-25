#pragma once

#include "Core.h"
#include "../Graphics/Texture.h"

namespace Pengine
{

	class PENGINE_API TextureManager
	{
	public:
		static TextureManager& GetInstance();

		std::shared_ptr<Texture> Create(const Texture::CreateInfo& createInfo);

		std::shared_ptr<Texture> Load(const std::string& filepath);

		std::vector<std::shared_ptr<Texture>> LoadFromFolder(const std::string& directory);

		std::shared_ptr<Texture> GetTexture(const std::string& filepath) const;

		std::shared_ptr<Texture> GetWhite() const;

		void ShutDown();

	private:
		TextureManager() = default;
		~TextureManager() = default;
		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;

		std::unordered_map<std::string, std::shared_ptr<Texture>> m_TexturesByFilepath;
	};

}