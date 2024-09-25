#include "VulkanFrameBuffer.h"

#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include "VulkanRenderPass.h"

#include "../Core/Logger.h"

using namespace Pengine;
using namespace Vk;

VulkanFrameBuffer::VulkanFrameBuffer(
	const std::vector<Texture::CreateInfo>& attachments,
	std::shared_ptr<RenderPass> renderPass,
	Renderer* renderer)
	: FrameBuffer(attachments, renderPass, renderer)
{
	if (m_AttachmentCreateInfos.empty())
	{
		FATAL_ERROR("Frame buffer attachments are empty!");
	}

	Resize(!m_AttachmentCreateInfos.empty() ? m_AttachmentCreateInfos[0].size : glm::ivec2{ 0, 0 });
}

VulkanFrameBuffer::~VulkanFrameBuffer()
{
	Clear();
}

void VulkanFrameBuffer::Resize(const glm::ivec2& size)
{
	Clear();

	m_Size = size;

	m_FrameBuffers.resize(swapChainImageCount);
	m_Attachments.resize(swapChainImageCount);

	const auto& renderPassAttachments = m_RenderPass->GetAttachmentDescriptions();

	uint32_t layers = 1;
	for (size_t frameIndex = 0; frameIndex < swapChainImageCount; frameIndex++)
	{
		std::vector<VkImageView> imageViews;
		size_t textureIndex = 0;
		for (Texture::CreateInfo& textureCreateInfo : m_AttachmentCreateInfos)
		{
			std::shared_ptr<Texture> texture;
			if (renderPassAttachments[textureIndex].getFrameBufferCallback && m_Renderer)
			{
				uint32_t attachmentIndex;
				const std::shared_ptr<FrameBuffer> frameBuffer = renderPassAttachments[textureIndex].getFrameBufferCallback(m_Renderer, attachmentIndex);
				texture = frameBuffer->GetAttachment(attachmentIndex, frameIndex);
				const std::shared_ptr<VulkanTexture> vkTexture = std::static_pointer_cast<VulkanTexture>(texture);
				imageViews.emplace_back(vkTexture->GetImageView());
			}
			else
			{
				textureCreateInfo.size = m_Size;

				texture = Texture::Create(textureCreateInfo);

				const std::shared_ptr<VulkanTexture> vkTexture = std::static_pointer_cast<VulkanTexture>(texture);
				imageViews.emplace_back(vkTexture->GetImageView());
				vkTexture->TransitionToRead();
			}
			
			m_Attachments[frameIndex].emplace_back(texture);

			layers = std::max(layers, texture->GetLayerCount());
			textureIndex++;
		}

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = std::static_pointer_cast<VulkanRenderPass>(m_RenderPass)->GetRenderPass();
		framebufferInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
		framebufferInfo.pAttachments = imageViews.data();
		framebufferInfo.width = static_cast<uint32_t>(m_Size.x);
		framebufferInfo.height = static_cast<uint32_t>(m_Size.y);
		framebufferInfo.layers = layers;

		if (vkCreateFramebuffer(
			device->GetDevice(),
			&framebufferInfo,
			nullptr,
			&m_FrameBuffers[frameIndex]) != VK_SUCCESS)
		{
			FATAL_ERROR("Failed to create framebuffer!");
		}
	}
}

void VulkanFrameBuffer::Clear()
{
	device->WaitIdle();

	for (const VkFramebuffer frameBuffer : m_FrameBuffers)
	{
		vkDestroyFramebuffer(device->GetDevice(), frameBuffer, nullptr);
	}
	
	m_FrameBuffers.clear();
	m_Attachments.clear();
}

void VulkanFrameBuffer::TransitionToRead() const
{
	for (const auto& attachment : m_Attachments[swapChainImageIndex])
	{
		const std::shared_ptr<VulkanTexture> vkTexture = std::static_pointer_cast<VulkanTexture>(attachment);
		vkTexture->TransitionToRead();
	}
}

void VulkanFrameBuffer::TransitionToColorAttachment() const
{
	for (const auto& attachment : m_Attachments[swapChainImageIndex])
	{
		const std::shared_ptr<VulkanTexture> vkTexture = std::static_pointer_cast<VulkanTexture>(attachment);
		vkTexture->TransitionToColorAttachment();
	}
}
