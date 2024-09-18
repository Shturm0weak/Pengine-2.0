#include "CSMRenderer.h"

using namespace Pengine;

bool CSMRenderer::GenerateLightSpaceMatrices(
	const glm::mat4& viewProjectionMat4,
	const glm::vec3 lightDirection,
	const float zNear,
	const float zFar,
	const int cascadeCount,
	const float splitFactor)
{
	m_Distances.clear();
	m_LightSpaceMatrices.clear();

	std::vector<float> splitDistances;
	splitDistances.resize(cascadeCount);

	const float clipRange = zFar - zNear;

	const float minZ = zNear;
	const float maxZ = zNear + clipRange;

	const float range = maxZ - minZ;
	const float ratio = maxZ / minZ;

	// Calculate split depths based on view camera frustum.
	// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	for (size_t i = 0; i < cascadeCount; i++)
	{
		float p = (i + 1) / static_cast<float>(cascadeCount);
		float log = minZ * std::pow(ratio, p);
		float uniform = minZ + range * p;
		float d = splitFactor * (log - uniform) + uniform;
		splitDistances[i] = (d - zNear) / clipRange;
	}

	float lastSplitDist = 0.0;
	for (size_t i = 0; i < cascadeCount; i++)
	{
		float splitDistance = splitDistances[i];
		glm::vec3 frustumCorners[8] =
		{
			glm::vec3(-1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f, -1.0f,  1.0f),
			glm::vec3(-1.0f, -1.0f,  1.0f),
		};

		// Project frustum corners into world space
		const glm::mat4 inverseViewProjectionMat4 = glm::inverse(viewProjectionMat4);
		for (size_t j = 0; j < 8; j++)
		{
			const glm::vec4 cornerWorldSpace = inverseViewProjectionMat4 * glm::vec4(frustumCorners[j], 1.0f);
			frustumCorners[j] = cornerWorldSpace / cornerWorldSpace.w;
		}

		for (size_t j = 0; j < 4; j++)
		{
			glm::vec3 distance = frustumCorners[j + 4] - frustumCorners[j];
			frustumCorners[j + 4] = frustumCorners[j] + (distance * splitDistance);
			frustumCorners[j] = frustumCorners[j] + (distance * lastSplitDist);
		}

		glm::vec3 frustumCenter = glm::vec3(0.0f);
		for (uint32_t j = 0; j < 8; j++)
		{
			frustumCenter += frustumCorners[j];
		}
		frustumCenter /= 8.0f;

		float radius = 0.0f;
		for (uint32_t j = 0; j < 8; j++)
		{
			float distance = glm::length(frustumCorners[j] - frustumCenter);
			radius = glm::max(radius, distance);
		}
		radius = std::ceil(radius * 16.0f) / 16.0f;

		glm::vec3 maxExtents = glm::vec3(radius);
		glm::vec3 minExtents = -maxExtents;

		glm::mat4 lightSpaceViewMat4 = glm::lookAt(frustumCenter + lightDirection * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightSpaceProjectionMat4 = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, minExtents.z - maxExtents.z, maxExtents.z - minExtents.z);

		m_Distances.emplace_back((zNear + splitDistance * clipRange));
		m_LightSpaceMatrices.emplace_back(lightSpaceProjectionMat4 * lightSpaceViewMat4);

		lastSplitDist = splitDistances[i];
	}
	
	const bool equalCascadeCounts = m_CascadeCount == cascadeCount;
	m_CascadeCount = cascadeCount;
	return !equalCascadeCounts;
}
