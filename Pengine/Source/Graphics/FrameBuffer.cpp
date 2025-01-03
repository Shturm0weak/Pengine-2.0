#include "FrameBuffer.h"

#include "../Core/Logger.h"
#include "../Vulkan/VulkanFrameBuffer.h"

using namespace Pengine;

std::shared_ptr<FrameBuffer> FrameBuffer::Create(
	std::shared_ptr<RenderPass> renderPass,
	RenderTarget* renderTarget,
	const glm::ivec2& size)
{
	if (!renderPass)
	{
		FATAL_ERROR("Render pass is nullptr!");
		return nullptr;
	}

	std::vector<Texture::CreateInfo> attachments;
	for (const auto& attachment : renderPass->GetAttachmentDescriptions())
	{
		const bool isColor = attachment.layout == Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

		Texture::CreateInfo attachmentCreateInfo{};
		attachmentCreateInfo.name = renderPass->GetType() + "FrameBuffer";
		attachmentCreateInfo.filepath = renderPass->GetType() + "FrameBuffer";
		attachmentCreateInfo.format = attachment.format;
		attachmentCreateInfo.channels = 4;
		attachmentCreateInfo.layerCount = attachment.layerCount;
		attachmentCreateInfo.isCubeMap = attachment.isCubeMap;
		attachmentCreateInfo.aspectMask = isColor ? Texture::AspectMask::COLOR :
			Texture::AspectMask::DEPTH;
		attachmentCreateInfo.usage = { Texture::Usage::SAMPLED,
			isColor ? Texture::Usage::COLOR_ATTACHMENT : Texture::Usage::DEPTH_STENCIL_ATTACHMENT };

		for (const auto& usage : attachment.usage)
		{
			attachmentCreateInfo.usage.emplace_back(usage);
		}

		attachmentCreateInfo.size = attachment.size ? *attachment.size : glm::ivec2((glm::vec2)size * renderPass->GetResizeViewportScale());
		attachmentCreateInfo.samplerCreateInfo = attachment.samplerCreateInfo;

		attachments.emplace_back(attachmentCreateInfo);
	}

	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanFrameBuffer>(attachments, renderPass, renderTarget);
	}

	FATAL_ERROR("Failed to create the framebuffer, no graphics API implementation");
	return nullptr;
}

FrameBuffer::FrameBuffer(
	const std::vector<Texture::CreateInfo>& attachments,
	std::shared_ptr<RenderPass> renderPass,
	RenderTarget* renderTarget)
	: m_RenderPass(std::move(renderPass))
	, m_AttachmentCreateInfos(attachments)
	, m_RenderTarget(renderTarget)
{
	if (attachments.empty())
	{
		FATAL_ERROR("Render pass attachments must contain at least one attachment!");
	}
}
