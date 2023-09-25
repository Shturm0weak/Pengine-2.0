#include "FrameBuffer.h"

#include "../Core/Logger.h"
#include "../Vulkan/VulkanFrameBuffer.h"

using namespace Pengine;

std::shared_ptr<FrameBuffer> Pengine::FrameBuffer::Create(std::shared_ptr<RenderPass> renderPass)
{
	if (!renderPass)
	{
		FATAL_ERROR("Render pass is nullptr!")
	}

	std::vector<Texture::CreateInfo> attachments;
	for (const auto& attachment : renderPass->GetAttachmentDescriptions())
	{
		const bool isColor = attachment.layout == Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

		Texture::CreateInfo attachmentCreateInfo{};
		attachmentCreateInfo.name = "RenderPassFrameBuffer";
		attachmentCreateInfo.filepath = none;
		attachmentCreateInfo.format = attachment.format;
		attachmentCreateInfo.data = nullptr;
		attachmentCreateInfo.channels = 4;
		attachmentCreateInfo.size = attachment.size;
		attachmentCreateInfo.aspectMask = isColor ? Texture::AspectMask::COLOR :
			Texture::AspectMask::DEPTH;
		attachmentCreateInfo.usage = { Texture::Usage::SAMPLED, 
			isColor ? Texture::Usage::COLOR_ATTACHMENT : Texture::Usage::DEPTH_STENCIL_ATTACHMENT };
		attachments.emplace_back(attachmentCreateInfo);
	}

	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanFrameBuffer>(attachments, renderPass);
	}
}

FrameBuffer::FrameBuffer(std::vector<Texture::CreateInfo> const& attachments,
	std::shared_ptr<RenderPass> renderPass)
	: m_RenderPass(renderPass)
	, m_AttachmentCreateInfos(attachments)
{
	if (attachments.empty())
	{
		FATAL_ERROR("Render pass attachments must contain at least one attachment!")
	}
}
