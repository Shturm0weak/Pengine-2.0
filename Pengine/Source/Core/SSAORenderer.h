#pragma once

#include "Core.h"

#include <array>

namespace Pengine
{

	class PENGINE_API SSAORenderer
	{
	public:
		void GenerateSamples(const int kernelSize);

		void GenerateNoiseTexture(const int noiseSize);

		void ShutDown();

		std::shared_ptr<class Texture> GetNoiseTexture() const { return m_NoiseTexture; }

		const std::array<glm::vec4, 64>& GetSamples() const { return m_Samples; }

		int m_NoiseSize = 0;
		int m_KernelSize = 0;

	private:
		std::array<glm::vec4, 64> m_Samples;

		std::shared_ptr<class Texture> m_NoiseTexture;

	};

}
