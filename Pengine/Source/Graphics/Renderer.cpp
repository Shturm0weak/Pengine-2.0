#include "Renderer.h"

#include "../Core/Camera.h"
#include "../Core/FileFormatNames.h"
#include "../Core/Logger.h"
#include "../Core/TextureManager.h"
#include "../Core/Time.h"
#include "../Core/RenderPassManager.h"
#include "../Core/Scene.h"
#include "../Core/SceneManager.h"
#include "../Core/Serializer.h"
#include "../Core/ViewportManager.h"
#include "../Core/WindowManager.h"
#include "../Components/Renderer3D.h"
#include "../Components/PointLight.h"
#include "../EventSystem/EventSystem.h"
#include "../EventSystem/ResizeEvent.h"
#include "../Vulkan/VulkanRenderer.h"
#include "../Vulkan/VulkanWindow.h"
#include "../Utils/Utils.h"

#include <filesystem>

using namespace Pengine;

std::shared_ptr<Renderer> Renderer::Create(const glm::ivec2& size)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanRenderer>(size);
	}
}

Renderer::Renderer(const glm::ivec2& size)
{
	for (const auto& type : renderPassesOrder)
	{
		if (std::shared_ptr<RenderPass> renderPass =
			RenderPassManager::GetInstance().GetRenderPass(type))
		{
			std::shared_ptr<FrameBuffer> frameBuffer = FrameBuffer::Create(renderPass);

			SetFrameBufferToRenderPass(type, frameBuffer);
		}
	}
}

Renderer::~Renderer()
{
	m_FrameBuffersByRenderPassType.clear();
}

void Renderer::Update(void* frame, std::shared_ptr<Window> window, std::shared_ptr<Camera> camera)
{
	for (const auto& type : renderPassesOrder)
	{
		std::shared_ptr<RenderPass> renderPass = RenderPassManager::GetInstance().GetRenderPass(type);
		if (!renderPass)
		{
			continue;
		}

		std::shared_ptr<FrameBuffer> frameBuffer = GetRenderPassFrameBuffer(renderPass->GetType());

		RenderPass::SubmitInfo renderPassSubmitInfo;
		renderPassSubmitInfo.width = frameBuffer->GetSize().x;
		renderPassSubmitInfo.height = frameBuffer->GetSize().y;
		renderPassSubmitInfo.frame = frame;
		renderPassSubmitInfo.frameBuffer = frameBuffer;
		renderPassSubmitInfo.renderPass = renderPass;

		BeginRenderPass(renderPassSubmitInfo);

		RenderPass::RenderCallbackInfo renderInfo{};
		renderInfo.renderer = this;
		renderInfo.camera = camera;
		renderInfo.window = window;
		renderInfo.submitInfo = renderPassSubmitInfo;

		renderPass->Render(renderInfo);

		EndRenderPass(renderPassSubmitInfo);
	}
}

std::shared_ptr<FrameBuffer> Renderer::GetRenderPassFrameBuffer(const std::string& type) const
{
	auto frameBufferByRenderPassType = m_FrameBuffersByRenderPassType.find(type);
	if (frameBufferByRenderPassType != m_FrameBuffersByRenderPassType.end())
	{
		return frameBufferByRenderPassType->second;
	}

	return nullptr;
}

void Renderer::SetFrameBufferToRenderPass(const std::string& type, std::shared_ptr<FrameBuffer> frameBuffer)
{
	if (!frameBuffer)
	{
		FATAL_ERROR("Frame buffer is nullptr!")
	}

	m_FrameBuffersByRenderPassType[type] = frameBuffer;
}

void Renderer::Resize(const glm::ivec2& size)
{
	if (std::shared_ptr<FrameBuffer> frameBuffer =
		GetRenderPassFrameBuffer(GBuffer))
	{
		frameBuffer->Resize(size);
	}

	if (std::shared_ptr<FrameBuffer> frameBuffer =
		GetRenderPassFrameBuffer(Deferred))
	{
		frameBuffer->Resize(size);
	}

}

void Renderer::FlushTextures(std::shared_ptr<BaseMaterial> baseMaterial,
	std::shared_ptr<Pipeline> pipeline, TextureSlots& globalTextureSlots)
{
	globalTextureSlots.Finish();

	if (pipeline)
	{
		if (std::shared_ptr<UniformWriter> uniformWriter = pipeline->GetUniformWriter())
		{
			if (std::shared_ptr<UniformLayout> uniformLayout = uniformWriter->GetLayout())
			{
				const auto& bindingsByLocation = uniformLayout->GetBindingsByLocation();
				for (auto [location, binding] : bindingsByLocation)
				{
					if (binding.type == UniformLayout::Type::SAMPLER_ARRAY)
					{
						if (!globalTextureSlots.textures.empty())
						{
							uniformWriter->WriteTextures(location, globalTextureSlots.textures);
						}
					}
				}
			}
		}
	}
}

bool Renderer::BindTexture(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<BaseMaterial> baseMaterial,
	std::shared_ptr<Material> material,
	TextureSlots& globalTextureSlots)
{
	/*if (!pipeline)
	{
		return true;
	}

	std::shared_ptr<UniformWriter> uniformWriter = pipeline->GetUniformWriter();

	if (!uniformWriter)
	{
		return true;
	}

	std::shared_ptr<UniformLayout> uniformLayout = uniformWriter->GetLayout();
	if (!uniformLayout)
	{
		return true;
	}

	const auto& bindingsByLocation = uniformLayout->GetBindingsByLocation();
	for (auto [location, binding] : bindingsByLocation)
	{
		if (binding.type == UniformLayout::Type::BUFFER)
		{
			std::shared_ptr<Buffer> buffer = uniformWriter->GetBufferByLocation(location);
			for (const UniformLayout::Variable& variable : binding.values)
			{
				if (variable.type == "sampler")
				{
					auto bindTexture = [&globalTextureSlots, &material, &buffer, variable](std::shared_ptr<Texture> texture) -> bool
					{
						const int slot = globalTextureSlots.BindTexture(texture);
						if (slot >= globalTextureSlots.textures.size())
						{
							return false;
						}
						else
						{
							buffer->Map(buffer->GetInstanceSize(), buffer->GetInstanceSize() * material->GetIndex());
							Utils::GetValue<int>(buffer->GetData(), variable.offset) = slot;
							buffer->Unmap();
						}
					};

					if (auto texture = Utils::Find(variable.name, material->GetTextures()))
					{
						if (!bindTexture(texture))
						{
							return false;
						}
					}
					else
					{
						auto renderPassAttachment = Utils::Find(variable.name, material->m_RenderPassAttachments);
						if (!renderPassAttachment.renderPassType.empty())
						{
							if (std::shared_ptr<FrameBuffer> frameBuffer = GetRenderPassFrameBuffer(renderPassAttachment.renderPassType))
							{
								if (!bindTexture(frameBuffer->GetAttachment(renderPassAttachment.attachment)))
								{
									return false;
								}
							}
						}
					}
				}
			}
		}
	}*/

	return true;
}