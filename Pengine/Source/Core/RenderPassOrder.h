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
		ZPrePass,
		GBuffer,
		CSM,
		ComputeSSAO,
		SSAOBlur,
		Deferred,
		Transparent,
		SSR,
		SSRBlur,
		Bloom,
		Final,
	};

}
