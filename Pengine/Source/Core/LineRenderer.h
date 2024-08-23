#pragma once

#include "Core.h"

#include "../Graphics/Buffer.h"
#include "../Graphics/RenderPass.h"

namespace Pengine
{

	class PENGINE_API LineRenderer
	{
	public:
		void Render(const RenderPass::RenderCallbackInfo& renderInfo);

		void ShutDown();
	private:
		struct Batch
		{
			std::shared_ptr<Buffer> m_VertexBuffer;
			std::shared_ptr<Buffer> m_IndexBuffer;
		};

		std::vector<Batch> m_Batches;
	};

}
