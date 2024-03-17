#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	class PENGINE_API PointLight
	{
	public:
		glm::vec3 color = { 1.0f, 1.0f, 1.0f };

		float constant = 1.0f;
		float linear = 0.09f;
		float quadratic = 0.032f;
	};

}
