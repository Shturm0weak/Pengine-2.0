#pragma once

#include "../Core/Core.h"

#include "Texture.h"
#include "RenderPass.h"

namespace Pengine
{

	class PENGINE_API FrameBuffer
	{
	public:
		static std::shared_ptr<FrameBuffer> Create(std::shared_ptr<RenderPass> renderPass);

		FrameBuffer(std::vector<Texture::CreateInfo> const& attachments,
			std::shared_ptr<RenderPass> renderPass);
		virtual ~FrameBuffer() = default;
		FrameBuffer(const FrameBuffer&) = delete;
		FrameBuffer& operator=(const FrameBuffer&) = delete;

		virtual std::shared_ptr<Texture> GetAttachment(size_t index) const = 0;

		virtual void Resize(const glm::ivec2& size) = 0;

		virtual void Clear() = 0;

		std::shared_ptr<RenderPass> GetRenderPass() const { return m_RenderPass; }

		glm::ivec2 GetSize() const { return m_Size; }

	protected:
		std::vector<Texture::CreateInfo> m_AttachmentCreateInfos;
		std::shared_ptr<RenderPass> m_RenderPass;
		glm::ivec2 m_Size;
	};

}