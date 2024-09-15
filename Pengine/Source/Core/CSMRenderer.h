#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API CSMRenderer
	{
	public:

		void GenerateLightSpaceMatrices(
			const glm::mat4& viewProjectionMat4,
			const glm::vec3 lightDirection,
			const float zNear,
			const float zFar,
			const int cascadeCount,
			const float splitFactor);

		[[nodiscard]] const std::vector<glm::mat4>& GetLightSpaceMatrices() const { return m_LightSpaceMatrices; }

		[[nodiscard]] const std::vector<float>& GetDistances() const { return m_Distances; }

	private:
		std::vector<glm::mat4> m_LightSpaceMatrices;
		std::vector<float> m_Distances;
	};

}
