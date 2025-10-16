#pragma once

#include "../Core/Core.h"
#include "../Core/UUID.h"

namespace Pengine
{

	class Mesh;
	class Material;

	class PENGINE_API Renderer3D
	{
	public:
		std::shared_ptr<Mesh> mesh;
		std::shared_ptr<Material> material;

		/**
		 * Used for transparency. From 0 to 11 [-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5].
		 */
		int renderingOrder = 5;

		bool isEnabled = true;
		bool castShadows = true;

		uint8_t objectVisibilityMask = -1;
		uint8_t shadowVisibilityMask = -1;

		UUID skeletalAnimatorEntity;

		~Renderer3D();
	};

}
