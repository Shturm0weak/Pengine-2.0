#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API Raycast
	{
	public:
		struct Hit
		{
			glm::vec3 point{};
			float distance = 0.0f;
		};

		static bool IntersectTriangle(
			const glm::vec3& start,
			const glm::vec3& direction,
			const glm::vec3& a,
			const glm::vec3& b,
			const glm::vec3& c,
			const glm::vec3& planeNormal,
			const float length,
			Hit& hit);

		static bool IntersectBoxOBB(
			const glm::vec3& start,
			const glm::vec3& direction,
			const glm::vec3& min,
			const glm::vec3& max,
			const glm::vec3& position,
			const glm::vec3& scale,
			const glm::mat4& rotation,
			const float length,
			Hit& hit);

		static std::map<float, std::shared_ptr<class Entity>> RaycastScene(
			std::shared_ptr<class Scene> scene,
			const glm::vec3& start,
			const glm::vec3& direction,
			const float length);

		typedef int OutCode;

		/**
		 * Compute the bit code for a point (x, y) using the clip rectangle bounded diagonally by min and max.
		 */
		static OutCode ComputeOutCode(
			const glm::vec2& point,
			const glm::vec2& min,
			const glm::vec2& max);

		/**
		 * Cohen–Sutherland clipping algorithm clips a line from start to end against a rectangle with diagonal from min to max.
		 */
		static bool CohenSutherlandLineClip(
			glm::vec2 start,
			glm::vec2 end,
			const glm::vec2& min,
			const glm::vec2& max);
	};

}
