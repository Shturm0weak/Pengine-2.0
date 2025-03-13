#pragma once

#include "Core.h"
#include "CustomData.h"
#include "Visualizer.h"

#include "../Graphics/Buffer.h"
#include "../Graphics/RenderPass.h"

namespace Pengine
{

	class PENGINE_API LineRenderer : public CustomData
	{
	public:
		virtual ~LineRenderer() override;
		
		void Render(const RenderPass::RenderCallbackInfo& renderInfo);

	private:
		struct Batch
		{
			std::shared_ptr<Buffer> vertexBuffer;
			std::shared_ptr<Buffer> indexBuffer;
		};

		std::vector<Batch> m_Batches;
	};

}
