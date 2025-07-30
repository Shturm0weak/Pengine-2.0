#pragma once

#include "Core/Core.h"

#include <queue>

namespace Pengine
{

	class PENGINE_API ThumbnailAtlas
	{
	public:
		struct TileInfo
		{
			void* atlasId = nullptr;
			int atlasIndex = -1;
			int tileIndex = -1;
			glm::vec2 uvStart;
			glm::vec2 uvEnd;
		};

		std::shared_ptr<class Texture> Push(
			std::shared_ptr<class Texture> texture,
			TileInfo& tileInfo);

		std::shared_ptr<class Texture> GetAtlas(int atlasIndex) const;

		void Free(int atlasIndex, int tileIndex);

	private:
		struct Atlas
		{
			std::shared_ptr<class Texture> texture;
			uint32_t count = 0;
			uint32_t tileSize = 0;
			std::queue<uint32_t> freeSlots;
		};

		std::vector<Atlas> m_Atlases;
	};

}
