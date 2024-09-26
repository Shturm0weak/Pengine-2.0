#include "FrustumCulling.h"

#include "Raycast.h"

#include "../Components/Renderer3D.h"
#include "../Components/Transform.h"
#include "../Graphics/Mesh.h"

using namespace Pengine;

bool FrustumCulling::CullBoundingBox(
	const glm::mat4& viewProjectionMat4,
	const glm::mat4& transformMat4,
	const glm::vec3& min,
	const glm::vec3& max,
	const float zNear)
{
	constexpr size_t pointCount = 8;
	constexpr size_t lineCount = 12;

	std::array<glm::vec4, pointCount> points;
	std::array<std::pair<glm::vec4, glm::vec4>, lineCount> lines;

	constexpr glm::vec3 frustumMin = { -1.0f, -1.0f, 0.0f };
	constexpr glm::vec3 frustumMax = { 1.0f,  1.0f, 0.0f };

	const glm::mat4 viewProjectionTransformMat4 = viewProjectionMat4 * transformMat4;

	points[0] = viewProjectionTransformMat4 * glm::vec4(min.x, min.y, min.z, 1.0f);
	points[1] = viewProjectionTransformMat4 * glm::vec4(max.x, min.y, min.z, 1.0f);
	points[2] = viewProjectionTransformMat4 * glm::vec4(max.x, max.y, min.z, 1.0f);
	points[3] = viewProjectionTransformMat4 * glm::vec4(min.x, max.y, min.z, 1.0f);
	points[4] = viewProjectionTransformMat4 * glm::vec4(min.x, min.y, max.z, 1.0f);
	points[5] = viewProjectionTransformMat4 * glm::vec4(max.x, min.y, max.z, 1.0f);
	points[6] = viewProjectionTransformMat4 * glm::vec4(max.x, max.y, max.z, 1.0f);
	points[7] = viewProjectionTransformMat4 * glm::vec4(min.x, max.y, max.z, 1.0f);

	int depthTestFailed = 0;
	for (size_t i = 0; i < pointCount; ++i)
	{
		points[i] /= glm::clamp<float>(points[i].w, zNear, FLT_MAX);
		depthTestFailed += points[i].z > 1.0f || points[i].z < 0.0f;
		points[i].z = 0.0f;
	}

	if (depthTestFailed == points.size())
	{
		return false;
	}

	// Front.
	lines[0] = std::make_pair(points[0], points[1]);
	lines[1] = std::make_pair(points[1], points[2]);
	lines[2] = std::make_pair(points[2], points[3]);
	lines[3] = std::make_pair(points[3], points[0]);

	// Back.
	lines[4] = std::make_pair(points[4], points[5]);
	lines[5] = std::make_pair(points[5], points[6]);
	lines[6] = std::make_pair(points[6], points[7]);
	lines[7] = std::make_pair(points[7], points[4]);

	// Left.
	lines[8] = std::make_pair(points[0], points[4]);
	lines[9] = std::make_pair(points[3], points[7]);

	// Right.
	lines[10] = std::make_pair(points[1], points[5]);
	lines[11] = std::make_pair(points[2], points[6]);

	int isEnabled = 0;
	for (size_t i = 0; i < lineCount; i++)
	{
		if (Raycast::CohenSutherlandLineClip(
			lines[i].first,
			lines[i].second,
			frustumMin,
			frustumMax))
		{
			isEnabled++;
		}
	}

	if (!isEnabled)
	{
		glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 aabbMax = glm::vec3(std::numeric_limits<float>::lowest());
		for (size_t i = 0; i < points.size(); i++)
		{
			const glm::vec3 point = points[i];
			aabbMin = (glm::min)(aabbMin, point);
			aabbMax = (glm::max)(aabbMax, point);
		}

		if (aabbMin.x < frustumMin.x && aabbMin.y < frustumMin.y
			&& aabbMax.x > frustumMax.x && aabbMax.y > frustumMax.y)
		{
			isEnabled++;
		}
		else
		{
			isEnabled *= 0;
		}
	}

	return (bool)isEnabled;
}
