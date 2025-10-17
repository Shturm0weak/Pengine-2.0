#pragma once

#include <string>
#include <vector>

namespace Pengine
{

	const std::vector<std::string> passPerSceneOrder =
	{
		Atmosphere
	};

	const std::vector<std::string> passPerViewportOrder =
	{
		UI,
		ZPrePass,
		GBuffer,
		Decals,
		CSM,
		SSAO,
		SSAOBlur,
		Deferred,
		Transparent,
		SSR,
		SSRBlur,
		Bloom,
		Final
	};

}
