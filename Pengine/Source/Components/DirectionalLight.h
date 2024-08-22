#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	class PENGINE_API DirectionalLight
	{
	public:
		glm::vec3 color = { 1.0f, 1.0f, 1.0f };
		float intensity = 1.0f;
	};

}
