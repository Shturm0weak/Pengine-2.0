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
			int kernelSize = 16;
			int noiseSize = 4;
			float radius = 0.5f;
			float bias = 0.025f;
			float aoScale = 2.0f;
		} ssao;
	};

}
