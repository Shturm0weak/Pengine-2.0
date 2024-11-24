#include "RenderTarget.h"

#include "../Core/Logger.h"
#include "../Core/RenderPassManager.h"
#include "../Core/Scene.h"
#include "../Core/Serializer.h"

#include <filesystem>

using namespace Pengine;

std::shared_ptr<RenderTarget> RenderTarget::Create(
	const std::vector<std::string>& renderPassOrder,
	const glm::ivec2& size)
{
	return std::make_shared<RenderTarget>(renderPassOrder, size);
}

RenderTarget::RenderTarget(
	const std::vector<std::string>& renderPassOrder,
	const glm::ivec2& size)
	: m_RenderPassOrder(renderPassOrder)
{
	for (const auto& type : m_RenderPassOrder)
	{
		if (std::shared_ptr<RenderPass> renderPass =
			RenderPassManager::GetInstance().GetRenderPass(type))
		{
			if (!renderPass->m_CreateFrameBuffer)
			{
				continue;
			}

			const std::shared_ptr<FrameBuffer> frameBuffer = FrameBuffer::Create(renderPass, this, size);

			SetFrameBufferToRenderPass(type, frameBuffer);
		}
	}
}

RenderTarget::~RenderTarget()
{
	m_FrameBuffersByRenderPassType.clear();
}

std::shared_ptr<UniformWriter> RenderTarget::GetUniformWriter(const std::string& renderPassName) const
{
	return Utils::Find(renderPassName, m_UniformWriterByRenderPassType);
}

void RenderTarget::SetUniformWriter(const std::string& renderPassName, std::shared_ptr<UniformWriter> uniformWriter)
{
	m_UniformWriterByRenderPassType[renderPassName] = uniformWriter;
}

std::shared_ptr<Buffer> RenderTarget::GetBuffer(const std::string& name) const
{
	return Utils::Find(name, m_BuffersByName);
}

void RenderTarget::SetBuffer(const std::string& name, std::shared_ptr<Buffer> buffer)
{
	m_BuffersByName[name] = buffer;
}

std::shared_ptr<FrameBuffer> RenderTarget::GetRenderPassFrameBuffer(const std::string& type) const
{
	if (const auto frameBufferByRenderPassType = m_FrameBuffersByRenderPassType.find(type);
		frameBufferByRenderPassType != m_FrameBuffersByRenderPassType.end())
	{
		return frameBufferByRenderPassType->second;
	}

	return nullptr;
}

void RenderTarget::SetFrameBufferToRenderPass(const std::string& type, const std::shared_ptr<FrameBuffer>& frameBuffer)
{
	if (!frameBuffer)
	{
		FATAL_ERROR("Frame buffer is nullptr!");
	}

	m_FrameBuffersByRenderPassType[type] = frameBuffer;
}

void RenderTarget::Resize(const glm::ivec2& size) const
{
	for (const std::string& renderPassName : m_RenderPassOrder)
	{
		if (const std::shared_ptr<FrameBuffer> frameBuffer =
			GetRenderPassFrameBuffer(renderPassName))
		{
			const std::shared_ptr<RenderPass> renderPass = RenderPassManager::GetInstance().GetRenderPass(renderPassName);
			if (renderPass && renderPass->GetResizeWithViewport())
			{
				frameBuffer->Resize(glm::vec2(size) * renderPass->GetResizeViewportScale());
			}
		}
	}
}
