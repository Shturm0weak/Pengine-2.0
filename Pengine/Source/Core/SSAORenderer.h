#pragma once

#include "Core.h"
#include "CustomData.h"

#include <array>

namespace Pengine
{

	class PENGINE_API SSAORenderer : public CustomData
	{
	public:
		virtual ~SSAORenderer() override;

		void GenerateSamples(const int kernelSize);

		void GenerateNoiseTexture(const int noiseSize);

		std::shared_ptr<class Texture> GetNoiseTexture() const { return m_NoiseTexture; }

		const std::array<glm::vec4, 64>& GetSamples() const { return m_Samples; }

		int GetNoiseSize() const { return m_NoiseSize; }

		int GetKernelSize() const { return m_KernelSize; }

	private:
		std::array<glm::vec4, 64> m_Samples;

		std::shared_ptr<class Texture> m_NoiseTexture;
		std::shared_ptr<class Texture> m_SSAOTexture;

		int m_NoiseSize = 0;
		int m_KernelSize = 0;
	};

}
