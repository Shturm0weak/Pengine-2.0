#pragma once

#include "../Core/Core.h"

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

		~Renderer3D();
	};

}
