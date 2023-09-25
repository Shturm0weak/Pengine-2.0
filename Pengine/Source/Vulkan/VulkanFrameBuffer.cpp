#include "VulkanFrameBuffer.h"

#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include "VulkanRenderPass.h"

#include "../Core/Logger.h"

using namespace Pengine;
using namespace Vk;

VulkanFrameBuffer::VulkanFrameBuffer(
    std::vector<Texture::CreateInfo> const& attachments,
    std::shared_ptr<RenderPass> renderPass)
	: FrameBuffer(attachments, renderPass)
{
    if (m_AttachmentCreateInfos.empty())
    {
        FATAL_ERROR("Frame buffer attachments are empty!");
    }

    Resize(attachments[0].size);
}

VulkanFrameBuffer::~VulkanFrameBuffer()
{
    Clear();
}

void VulkanFrameBuffer::Resize(const glm::ivec2& size)
{
    if (vkDeviceWaitIdle(device->GetDevice()) != VK_SUCCESS)
    {
        Logger::Warning("Can't wait device idle to recreate frame buffer!");
    }

    Clear();

    m_Size = size;

    m_FrameBuffers.resize(swapChainImageCount);
    m_Attachments.resize(swapChainImageCount);

    for (size_t frameIndex = 0; frameIndex < swapChainImageCount; frameIndex++)
    {
        std::vector<VkImageView> imageViews;
        for (Texture::CreateInfo& textureCreateInfo : m_AttachmentCreateInfos)
        {
            textureCreateInfo.size = m_Size;

            std::shared_ptr<Texture> texture = Texture::Create(textureCreateInfo);
            m_Attachments[frameIndex].emplace_back(texture);

            std::shared_ptr<VulkanTexture> vkTexture = std::static_pointer_cast<VulkanTexture>(texture);
            imageViews.emplace_back(vkTexture->GetImageView());
            vkTexture->TransitionToRead();
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = std::static_pointer_cast<VulkanRenderPass>(m_RenderPass)->GetRenderPass();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
        framebufferInfo.pAttachments = imageViews.data();
        framebufferInfo.width = static_cast<uint32_t>(m_Size.x);
        framebufferInfo.height = static_cast<uint32_t>(m_Size.y);
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
            device->GetDevice(),
            &framebufferInfo,
            nullptr,
            &m_FrameBuffers[frameIndex]) != VK_SUCCESS)
        {
            FATAL_ERROR("Failed to create framebuffer!")
        }
    }
}

void VulkanFrameBuffer::Clear()
{
    vkDeviceWaitIdle(device->GetDevice());

    for (VkFramebuffer frameBuffer : m_FrameBuffers)
    {
        vkDestroyFramebuffer(device->GetDevice(), frameBuffer, nullptr);
    }

    m_FrameBuffers.clear();
    m_Attachments.clear();
}

void VulkanFrameBuffer::TransitionToRead()
{
    for (size_t imageIndex = 0; imageIndex < m_Attachments[swapChainImageIndex].size(); imageIndex++)
    {
        std::shared_ptr<VulkanTexture> vkTexture = std::static_pointer_cast<VulkanTexture>(m_Attachments[swapChainImageIndex][imageIndex]);
        vkTexture->TransitionToRead();
    }
}

void VulkanFrameBuffer::TransitionToColorAttachment()
{
    for (size_t imageIndex = 0; imageIndex < m_Attachments[swapChainImageIndex].size(); imageIndex++)
    {
        std::shared_ptr<VulkanTexture> vkTexture = std::static_pointer_cast<VulkanTexture>(m_Attachments[swapChainImageIndex][imageIndex]);
        vkTexture->TransitionToColorAttachment();
    }
}