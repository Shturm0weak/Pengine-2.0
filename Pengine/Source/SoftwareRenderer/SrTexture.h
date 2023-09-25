#pragma once

#include "../Core/Core.h"
#include "../Graphics/Texture.h"

namespace Pengine
{

	class PENGINE_API SrTexture : public Texture
	{
	public:
		SrTexture(CreateInfo textureCreateInfo);
		~SrTexture() = default;
		SrTexture(const SrTexture&) = delete;
		SrTexture& operator=(const SrTexture&) = delete;

		glm::ivec4 Sample(glm::vec2 uv) const;

	private:
	};

}