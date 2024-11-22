#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	struct PENGINE_API Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};

	struct PENGINE_API VertexForShadows
	{
		glm::vec3 position;
		glm::vec2 uv;
	};

}
