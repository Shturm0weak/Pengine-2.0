#pragma once

#include <string>
#include <vector>

namespace Pengine
{

	const std::vector<std::string> renderPassPerSceneOrder =
	{
		Atmosphere
	};

	const std::vector<std::string> renderPassPerViewportOrder =
	{
		ZPrePass,
		GBuffer,
		CSM,
		SSAO,
		SSAOBlur,
		Deferred,
		Transparent,
		SSR,
		SSRBlur,
		Bloom,
		Final,
	};

}
