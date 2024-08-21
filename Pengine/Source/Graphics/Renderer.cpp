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

Renderer::Renderer()
{
	for (const auto& type : renderPassesOrder)
	{
		if (std::shared_ptr<RenderPass> renderPass =
			RenderPassManager::GetInstance().GetRenderPass(type))
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = FrameBuffer::Create(renderPass);

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
	const std::shared_ptr<Entity>& camera)
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

		const std::shared_ptr<FrameBuffer> frameBuffer = GetRenderPassFrameBuffer(renderPass->GetType());

		RenderPass::SubmitInfo renderPassSubmitInfo;
		renderPassSubmitInfo.width = frameBuffer->GetSize().x;
		renderPassSubmitInfo.height = frameBuffer->GetSize().y;
		renderPassSubmitInfo.frame = frame;
		renderPassSubmitInfo.frameBuffer = frameBuffer;
		renderPassSubmitInfo.renderPass = renderPass;

		BeginRenderPass(renderPassSubmitInfo);

		RenderPass::RenderCallbackInfo renderInfo{};
		renderInfo.renderer = shared_from_this();
		renderInfo.camera = camera;
		renderInfo.window = window;
		renderInfo.scene = scene;
		renderInfo.submitInfo = renderPassSubmitInfo;

		renderPass->Render(renderInfo);

		EndRenderPass(renderPassSubmitInfo);
	}
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
	if (const std::shared_ptr<FrameBuffer> frameBuffer =
		GetRenderPassFrameBuffer(GBuffer))
	{
		frameBuffer->Resize(size);
	}

	if (const std::shared_ptr<FrameBuffer> frameBuffer =
		GetRenderPassFrameBuffer(Deferred))
	{
		frameBuffer->Resize(size);
	}
}