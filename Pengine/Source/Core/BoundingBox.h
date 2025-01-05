#pragma once

#include "Core.h"

namespace Pengine
{

	struct BoundingBox
	{
		glm::vec3 max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
		glm::vec3 min = { FLT_MAX, FLT_MAX, FLT_MAX };
		glm::vec3 offset = { 0.0f, 0.0f, 0.0f };
	};

}
