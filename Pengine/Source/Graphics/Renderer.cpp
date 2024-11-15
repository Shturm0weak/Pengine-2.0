#include "Renderer.h"

#include "../Core/Logger.h"
#include "../Core/RenderPassManager.h"
#include "../Core/Scene.h"
#include "../Core/Serializer.h"
#include "../Vulkan/VulkanRenderer.h"
#include "../Vulkan/VulkanWindow.h"

#include <filesystem>

using namespace Pengine;

std::shared_ptr<Renderer> Renderer::Create(const glm::ivec2& size)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanRenderer>(size);
	}

	FATAL_ERROR("Failed to create the renderer, no graphics API implementation");
	return nullptr;
}

Renderer::Renderer(const glm::ivec2& size)
{
	for (const auto& type : renderPassesOrder)
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

Renderer::~Renderer()
{
	m_FrameBuffersByRenderPassType.clear();
}

void Renderer::Update(
	void* frame,
	const std::shared_ptr<Window>& window,
	const std::shared_ptr<Scene>& scene,
	const std::shared_ptr<Entity>& camera,
	const glm::mat4& projection,
	const glm::ivec2& viewportSize)
{
	if (!scene)
	{
		return;
	}

	for (const auto& type : renderPassesOrder)
	{
		const std::shared_ptr<RenderPass> renderPass = RenderPassManager::GetInstance().GetRenderPass(type);
		if (!renderPass)
		{
			continue;
		}

		if (!renderPass->m_IsInitialized)
		{
			if (renderPass->m_CreateCallback)
			{
				renderPass->m_CreateCallback(*renderPass.get());
				renderPass->m_IsInitialized = true;
			}
		}

		RenderPass::RenderCallbackInfo renderInfo{};
		renderInfo.renderer = shared_from_this();
		renderInfo.camera = camera;
		renderInfo.window = window;
		renderInfo.scene = scene;
		renderInfo.renderPass = renderPass;
		renderInfo.projection = projection;
		renderInfo.frame = frame;
		renderInfo.viewportSize = viewportSize;

		renderPass->Render(renderInfo);
	}
}

std::shared_ptr<UniformWriter> Renderer::GetUniformWriter(const std::string& renderPassName) const
{
	return Utils::Find(renderPassName, m_UniformWriterByRenderPassType);
}

void Renderer::SetUniformWriter(const std::string& renderPassName, std::shared_ptr<UniformWriter> uniformWriter)
{
	m_UniformWriterByRenderPassType[renderPassName] = uniformWriter;
}

std::shared_ptr<Buffer> Renderer::GetBuffer(const std::string& name) const
{
	return Utils::Find(name, m_BuffersByName);
}

void Renderer::SetBuffer(const std::string& name, std::shared_ptr<Buffer> buffer)
{
	m_BuffersByName[name] = buffer;
}

std::shared_ptr<FrameBuffer> Renderer::GetRenderPassFrameBuffer(const std::string& type) const
{
	if (const auto frameBufferByRenderPassType = m_FrameBuffersByRenderPassType.find(type);
		frameBufferByRenderPassType != m_FrameBuffersByRenderPassType.end())
	{
		return frameBufferByRenderPassType->second;
	}

	return nullptr;
}

void Renderer::SetFrameBufferToRenderPass(const std::string& type, const std::shared_ptr<FrameBuffer>& frameBuffer)
{
	if (!frameBuffer)
	{
		FATAL_ERROR("Frame buffer is nullptr!");
	}

	m_FrameBuffersByRenderPassType[type] = frameBuffer;
}

void Renderer::Resize(const glm::ivec2& size) const
{
	for (const std::string& renderPassName : renderPassesOrder)
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
