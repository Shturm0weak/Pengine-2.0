#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	struct PENGINE_API VertexDefault
	{
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};

	struct PENGINE_API VertexPosition
	{
		glm::vec3 position;
		glm::vec2 uv;
	};

	struct PENGINE_API VertexNormal
	{
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};

	struct PENGINE_API VertexLayout
	{
		uint32_t size;
	};

}
