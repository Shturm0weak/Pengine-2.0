#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	class PENGINE_API SpotLight
	{
	public:
		glm::vec3 color = { 1.0f, 1.0f, 1.0f };

		float intensity = 1.0f;
		float radius = 1.0f;
		float bias = 0.01f;

		float innerCutOff = glm::radians(45.0f);
		float outerCutOff = glm::radians(45.0f);

		bool drawBoundingSphere = false;
		bool castShadows = true;
		bool castSSS = false;
	};

}
