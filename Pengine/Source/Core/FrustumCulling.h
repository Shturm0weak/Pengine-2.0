#pragma once

#include "Core.h"
#include "Scene.h"

namespace Pengine
{

	class PENGINE_API FrustumCulling
	{
	public:
		static bool CullBoundingBox(
			const glm::mat4& viewProjectionMat4,
			const glm::mat4& transformMat4,
			const glm::vec3& min,
			const glm::vec3& max,
			const float zNear);
	};

}
