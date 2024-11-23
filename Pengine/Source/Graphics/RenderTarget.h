#pragma once

#include "../Core/Core.h"
#include "../Graphics/Buffer.h"
#include "../Graphics/FrameBuffer.h"
#include "../Graphics/UniformWriter.h"

namespace Pengine
{
	class PENGINE_API RenderTarget
	{
	public:
		static std::shared_ptr<RenderTarget> Create(const glm::ivec2& size);

		explicit RenderTarget(const glm::ivec2& size);
		virtual ~RenderTarget();
		RenderTarget(const RenderTarget&) = delete;
		RenderTarget(RenderTarget&&) = delete;
		RenderTarget& operator=(const RenderTarget&) = delete;
		RenderTarget& operator=(RenderTarget&&) = delete;

		std::shared_ptr<UniformWriter> GetUniformWriter(const std::string& renderPassName) const;

		void SetUniformWriter(const std::string& renderPassName, std::shared_ptr<UniformWriter> uniformWriter);

		std::shared_ptr<Buffer> GetBuffer(const std::string& name) const;

		void SetBuffer(const std::string& name, std::shared_ptr<Buffer> buffer);

		std::shared_ptr<FrameBuffer> GetRenderPassFrameBuffer(const std::string& type) const;

		void SetFrameBufferToRenderPass(const std::string& type,
			const std::shared_ptr<FrameBuffer>& frameBuffer);

		void Resize(const glm::ivec2& size) const;

	protected:
		std::unordered_map<std::string, std::shared_ptr<FrameBuffer>> m_FrameBuffersByRenderPassType;
		std::unordered_map<std::string, std::shared_ptr<UniformWriter>> m_UniformWriterByRenderPassType;
		std::unordered_map<std::string, std::shared_ptr<Buffer>> m_BuffersByName;
	};

}