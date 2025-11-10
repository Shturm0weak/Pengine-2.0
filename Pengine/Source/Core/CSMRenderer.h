#pragma once

#include "Core.h"
#include "CustomData.h"

namespace Pengine
{

	class PENGINE_API CSMRenderer : public CustomData
	{
	public:
		virtual ~CSMRenderer() override = default;
		/**
		 * @return whether the frame buffer has to be recreated due to changed cascade count.
		 */
		bool GenerateLightSpaceMatrices(
			const glm::mat4& viewProjectionMat4,
			const glm::vec3& lightDirection,
			const glm::vec2& shadowMapSize,
			const float zNear,
			const float zFar,
			const int cascadeCount,
			const float splitFactor,
			const bool stabilizeCascades);

		[[nodiscard]] const std::vector<glm::mat4>& GetLightSpaceMatrices() const { return m_LightSpaceMatrices; }

		[[nodiscard]] const std::vector<float>& GetDistances() const { return m_Distances; }

	private:
		std::vector<glm::mat4> m_LightSpaceMatrices;
		std::vector<float> m_Distances;

		int m_CascadeCount = 0;
	};

}
