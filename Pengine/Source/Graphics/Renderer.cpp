#include "Renderer.h"

#include "../Core/Logger.h"
#include "../Core/RenderPassManager.h"
#include "../Core/RenderPassOrder.h"
#include "../Core/Scene.h"
#include "../Core/Serializer.h"
#include "../Vulkan/VulkanRenderer.h"
#include "../Vulkan/VulkanWindow.h"

#include <filesystem>

using namespace Pengine;

std::shared_ptr<Renderer> Renderer::Create()
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanRenderer>();
	}

	FATAL_ERROR("Failed to create the renderer, no graphics API implementation");
	return nullptr;
}

void Renderer::Update(
	void* frame,
	const std::shared_ptr<Window>& window,
	const std::shared_ptr<Renderer>& renderer,
	const std::map<std::shared_ptr<Scene>, std::vector<RenderViewportInfo>>& viewportsByScene)
{
	for (const auto& [scene, viewports] : viewportsByScene)
	{
		if (!scene)
		{
			return;
		}

		if (!scene->GetRenderTarget())
		{
			scene->SetRenderTarget(RenderTarget::Create(renderPassPerSceneOrder, { 0, 0 }));
		}

		RenderPass::RenderCallbackInfo renderInfo{};
		renderInfo.renderer = renderer;
		renderInfo.camera = nullptr;
		renderInfo.window = window;
		renderInfo.scene = scene;
		renderInfo.projection = glm::mat4(1.0f);
		renderInfo.frame = frame;
		renderInfo.viewportSize = { 0, 0 };

		for (const auto& type : renderPassPerSceneOrder)
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

			renderInfo.renderPass = renderPass;

			renderPass->Render(renderInfo);
		}
		
		for (const auto& viewport : viewports)
		{
			renderInfo.camera = viewport.camera;
			renderInfo.projection = viewport.projection;
			renderInfo.viewportSize = viewport.size;
			renderInfo.renderTarget = viewport.renderTarget;

			RenderPassManager::PrepareUniformsPerViewportBeforeDraw(renderInfo);

			for (const auto& type : renderPassPerViewportOrder)
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

				renderInfo.renderPass = renderPass;

				renderPass->Render(renderInfo);
			}
		}
	}
}
