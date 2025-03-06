#pragma once

#include "../Core/Core.h"

#include "Pass.h"

namespace Pengine
{

	class PENGINE_API ComputePass final : public Pass
	{
	  public:
		struct CreateInfo
		{
			Type type;
			std::string name;
			std::function<void(const RenderCallbackInfo&)> executeCallback;
			std::function<void(Pass*)> createCallback;
		};

		explicit ComputePass(const CreateInfo& createInfo);
		virtual ~ComputePass() override = default;
		ComputePass(const ComputePass&) = delete;
		ComputePass(ComputePass&&) = delete;
		ComputePass& operator=(const ComputePass&) = delete;
		ComputePass& operator=(ComputePass&&) = delete;

		virtual void Execute(const RenderCallbackInfo& renderInfo) const override;
	};

}
