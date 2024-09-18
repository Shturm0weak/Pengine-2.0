#include "RenderPass.h"

#include "../Core/Logger.h"
#include "../Utils/Utils.h"
#include "../Vulkan/VulkanRenderPass.h"

using namespace Pengine;

std::shared_ptr<RenderPass> RenderPass::Create(const CreateInfo& createInfo)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanRenderPass>(createInfo);
	}

	FATAL_ERROR("Failed to create the renderer, no graphics API implementation");
	return nullptr;
}

RenderPass::RenderPass(const CreateInfo& createInfo)
	: m_AttachmentDescriptions(createInfo.attachmentDescriptions)
	, m_ClearColors(createInfo.clearColors)
	, m_ClearDepths(createInfo.clearDepths)
	, m_Type(createInfo.type)
	, m_RenderCallback(createInfo.renderCallback)
	, m_CreateCallback(createInfo.createCallback)
	, m_ResizeViewportScale(createInfo.resizeViewportScale)
	, m_ResizeWithViewport(createInfo.resizeWithViewport)
	, m_CreateFrameBuffer(createInfo.createFrameBuffer)
{
}

std::shared_ptr<Buffer> RenderPass::GetBuffer(const std::string& name) const
{
	return Utils::Find(name, m_BuffersByName);
}

void RenderPass::SetBuffer(const std::string& name, const std::shared_ptr<Buffer>& buffer)
{
	m_BuffersByName[name] = buffer;
}

void RenderPass::SetUniformWriter(std::shared_ptr<UniformWriter> uniformWriter)
{
	m_UniformWriter = uniformWriter;
}

void RenderPass::Render(const RenderCallbackInfo& renderInfo) const
{
	if (m_RenderCallback)
	{
		m_RenderCallback(renderInfo);
	}
}
