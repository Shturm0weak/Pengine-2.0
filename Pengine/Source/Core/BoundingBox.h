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

	struct AABB
	{
		glm::vec3 min, max;

		AABB()
		{
			min = { std::numeric_limits<float>::max(),
					std::numeric_limits<float>::max(),
					std::numeric_limits<float>::max() };
			max = { std::numeric_limits<float>::lowest(),
					std::numeric_limits<float>::lowest(),
					std::numeric_limits<float>::lowest() };
		}

		AABB(const glm::vec3& min, const glm::vec3& max)
			: min(min)
			, max(max)
		{
		}

		AABB Expanded(const AABB& aabb) const
		{
			return AABB(
				{ std::min(min.x, aabb.min.x), std::min(min.y, aabb.min.y), std::min(min.z, aabb.min.z) },
				{ std::max(max.x, aabb.max.x), std::max(max.y, aabb.max.y), std::max(max.z, aabb.max.z) }
			);
		}

		glm::vec3 Center() const
		{
			return
			{
				(min.x + max.x) * 0.5f,
				(min.y + max.y) * 0.5f,
				(min.z + max.z) * 0.5f
			};
		}

		float SurfaceArea() const
		{
			glm::vec3 d = { max.x - min.x, max.y - min.y, max.z - min.z };
			return 2.0f * (d.x * d.y + d.x * d.z + d.y * d.z);
		}
	};

}
