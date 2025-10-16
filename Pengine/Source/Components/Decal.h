#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	class Material;
	
	class PENGINE_API Decal
	{
	public:
		std::shared_ptr<Material> material;
		uint8_t objectVisibilityMask = -1;

		~Decal();
	};

}
