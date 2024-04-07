#pragma once

#include <shaderc/shaderc.hpp>

namespace Pengine::Vk
{

	class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
	{
	public:
		[[nodiscard]] virtual shaderc_include_result* GetInclude(
			const char* requestedSource,
			shaderc_include_type type,
			const char* requestingSource,
			size_t includeDepth) override;

		virtual void ReleaseInclude(shaderc_include_result* data) override;
	};

}
