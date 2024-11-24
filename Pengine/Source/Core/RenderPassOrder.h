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
		GBuffer,
		CSM,
		SSAO,
		SSAOBlur,
		Deferred,
		Transparent,
		Final,
	};

}
