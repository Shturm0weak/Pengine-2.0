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
	for (const auto& name : m_RenderPassOrder)
	{
		if (std::shared_ptr<Pass> pass =
			RenderPassManager::GetInstance().GetPass(name))
		{
			if (pass->GetType() == Pass::Type::COMPUTE)
			{
				continue;
			}

			std::shared_ptr<RenderPass> renderPass = std::dynamic_pointer_cast<RenderPass>(pass);
			if (!renderPass->m_CreateFrameBuffer)
			{
				continue;
			}

			const std::shared_ptr<FrameBuffer> frameBuffer = FrameBuffer::Create(renderPass, this, size);

			SetFrameBuffer(name, frameBuffer);
		}
	}
}

RenderTarget::~RenderTarget()
{
	for (auto& [name, data] : m_CustomDataByName)
	{
		delete data;
	}
	m_CustomDataByName.clear();
	
	m_FrameBuffersByName.clear();
	m_UniformWriterByName.clear();
	m_BuffersByName.clear();
}

std::shared_ptr<UniformWriter> RenderTarget::GetUniformWriter(const std::string& renderPassName) const
{
	return Utils::Find(renderPassName, m_UniformWriterByName);
}

void RenderTarget::SetUniformWriter(const std::string& renderPassName, std::shared_ptr<UniformWriter> uniformWriter)
{
	m_UniformWriterByName[renderPassName] = uniformWriter;
}

std::shared_ptr<Buffer> RenderTarget::GetBuffer(const std::string& name) const
{
	return Utils::Find(name, m_BuffersByName);
}

void RenderTarget::SetBuffer(const std::string& name, std::shared_ptr<Buffer> buffer)
{
	m_BuffersByName[name] = buffer;
}

std::shared_ptr<FrameBuffer> RenderTarget::GetFrameBuffer(const std::string& name) const
{
	if (const auto frameBuffersByName = m_FrameBuffersByName.find(name);
		frameBuffersByName != m_FrameBuffersByName.end())
	{
		return frameBuffersByName->second;
	}

	return nullptr;
}

void RenderTarget::SetFrameBuffer(const std::string& name, const std::shared_ptr<FrameBuffer>& frameBuffer)
{
	if (!frameBuffer)
	{
		FATAL_ERROR("Frame buffer is nullptr!");
	}

	m_FrameBuffersByName[name] = frameBuffer;
}

void RenderTarget::DeleteFrameBuffer(const std::string& name)
{
	if (const auto frameBuffersByName = m_FrameBuffersByName.find(name);
		frameBuffersByName != m_FrameBuffersByName.end())
	{
		frameBuffersByName->second = nullptr;
	}
}

CustomData* RenderTarget::GetCustomData(const std::string& name)
{
	if (const auto customDataByName = m_CustomDataByName.find(name);
		customDataByName != m_CustomDataByName.end())
	{
		return customDataByName->second;
	}

	return nullptr;
}

void RenderTarget::SetCustomData(const std::string& name, CustomData* data)
{
	if (!data)
	{
		FATAL_ERROR("Custom data is nullptr!");
	}

	m_CustomDataByName[name] = data;
}

void RenderTarget::Resize(const glm::ivec2& size) const
{
	for (const std::string& renderPassName : m_RenderPassOrder)
	{
		if (const std::shared_ptr<FrameBuffer> frameBuffer =
			GetFrameBuffer(renderPassName))
		{
			const std::shared_ptr<RenderPass> renderPass = RenderPassManager::GetInstance().GetRenderPass(renderPassName);
			if (renderPass && renderPass->GetResizeWithViewport())
			{
				frameBuffer->Resize(glm::vec2(size) * renderPass->GetResizeViewportScale());
			}
		}
	}
}
