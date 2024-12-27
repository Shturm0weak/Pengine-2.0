#pragma once

#include "Core.h"
#include "Asset.h"

namespace Pengine
{

	class PENGINE_API GraphicsSettings : public Asset
	{
	public:
		GraphicsSettings(
			const std::string& name,
			const std::filesystem::path& filepath)
			: Asset(name, filepath)
		{}

		struct SSAO
		{
			bool isEnabled = true;
			int kernelSize = 16;
			int noiseSize = 4;
			float radius = 0.5f;
			float bias = 0.025f;
			float aoScale = 2.0f;
		} ssao;

		struct Shadows
		{
			bool isEnabled = true;
			std::vector<float> biases = { 0.2f, 0.5f, 0.2f };
			float splitFactor = 0.75f;
			float maxDistance = 200.0f;
			float fogFactor = 0.2f;
			bool pcfEnabled = true;
			bool visualize = false;
			int pcfRange = 1;

			/**
			 * Resolution: 1024 / 2048 / 4096
			 * 1024 by default.
			 */
			int quality = 1;
			
			/**
			 * Has to be more than 1.
			 */
			int cascadeCount = 3;
		} shadows;

		struct Bloom
		{
			bool isEnabled = true;
			int mipCount = 8;
			float brightnessThreshold = 1.0f;
		} bloom;

		struct PostProcess
		{
			enum class ToneMapper : int
			{
				NONE,
				ACES,
				COUNT
			};

			ToneMapper toneMapper = ToneMapper::ACES;
			float gamma = 2.2f;
			bool fxaa = true;
		} postProcess;
	};

}
