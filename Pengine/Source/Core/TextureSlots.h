#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API TextureSlots
	{
	public:
		TextureSlots(size_t size = MAX_TEXTURES);

		int BindTexture(std::shared_ptr<class Texture> texture, std::optional<int> slot = std::nullopt);

		void ResetTextures();

		void Finish();

		std::vector<std::shared_ptr<class Texture>> textures;

	private:
		int m_CurrentSlot = 0;
	};

}