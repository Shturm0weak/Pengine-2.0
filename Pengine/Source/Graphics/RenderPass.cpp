#include "RenderPass.h"

#include "../Utils/Utils.h"
#include "../Vulkan/VulkanRenderPass.h"

using namespace Pengine;

std::shared_ptr<RenderPass> RenderPass::Create(const CreateInfo& createInfo)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanRenderPass>(createInfo);
	}
}

RenderPass::RenderPass(const CreateInfo& createInfo)
	: m_Type(createInfo.type)
	, m_ClearColors(createInfo.clearColors)
	, m_ClearDepths(createInfo.clearDepths)
	, m_AttributeDescriptions(createInfo.attributeDescriptions)
	, m_BindingDescriptions(createInfo.bindingDescriptions)
	, m_AttachmentDescriptions(createInfo.attachmentDescriptions)
	, m_RenderCallback(createInfo.renderCallback)
	, m_CreateCallback(createInfo.createCallback)
	, m_BuffersByName(createInfo.buffersByName)
{
	if (!createInfo.uniformBindings.empty())
	{
		m_UniformWriter = UniformWriter::Create(UniformLayout::Create(createInfo.uniformBindings));
	}

	if (m_CreateCallback)
	{
		m_CreateCallback(*this);
	}
}

std::shared_ptr<Buffer> RenderPass::GetBuffer(const std::string& name) const
{
	return Utils::Find(name, m_BuffersByName);
}

void RenderPass::SetBuffer(const std::string& name, std::shared_ptr<Buffer> buffer)
{
	m_BuffersByName[name] = buffer;
}

void RenderPass::Render(RenderCallbackInfo renderInfo)
{
	if (m_RenderCallback)
	{
		m_RenderCallback(renderInfo);
	}
}
