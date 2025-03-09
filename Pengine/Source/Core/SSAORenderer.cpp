#include "SSAORenderer.h"

#include "TextureManager.h"

#include <random>

using namespace Pengine;

SSAORenderer::~SSAORenderer()
{
	m_NoiseTexture = nullptr;
}

void SSAORenderer::GenerateSamples(const int kernelSize)
{
	m_KernelSize = kernelSize;

	auto ourLerp = [](float a, float b, float f)
	{
		return a + f * (b - a);
	};

	std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
	std::default_random_engine generator;
	for (int i = 0; i < m_KernelSize; ++i)
	{
		glm::vec4 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator), 1.0f);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / (float)m_KernelSize;
		scale = ourLerp(0.1f, 1.0f, scale * scale);
		sample *= scale;

		m_Samples[i] = sample;
	}
}

void SSAORenderer::GenerateNoiseTexture(const int noiseSize)
{
	m_NoiseSize = noiseSize;
	m_NoiseTexture = nullptr;

	std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
	std::default_random_engine generator;

	std::vector<glm::vec4> noiseData;
	for (int i = 0; i < m_NoiseSize * m_NoiseSize; i++)
	{
		glm::vec4 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f, 0.0f);
		noiseData.push_back(noise);
	}

	Texture::CreateInfo noiseCreateInfo{};
	noiseCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	noiseCreateInfo.channels = 4;
	noiseCreateInfo.filepath = "NoiseTexture";
	noiseCreateInfo.name = "NoiseTexture";
	noiseCreateInfo.format = Format::R32G32B32A32_SFLOAT;
	noiseCreateInfo.instanceSize = sizeof(float);
	noiseCreateInfo.isCubeMap = false;
	noiseCreateInfo.mipLevels = 1;
	noiseCreateInfo.size = { m_NoiseSize, m_NoiseSize };
	noiseCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_DST };
	noiseCreateInfo.data = noiseData.data();
	
	Texture::SamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.filter = Texture::SamplerCreateInfo::Filter::NEAREST;

	noiseCreateInfo.samplerCreateInfo = samplerCreateInfo;

	m_NoiseTexture = Texture::Create(noiseCreateInfo);
}
