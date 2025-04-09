#pragma once

#include "../Core/Core.h"
#include "../Core/Window.h"

#include "VulkanFrameInfo.h"

namespace Pengine::Vk
{

	class PENGINE_API VulkanHeadlessWindow final : public Window
	{
	public:
		VulkanHeadlessWindow(const std::string& title, const std::string& name, const glm::ivec2& size);
		virtual ~VulkanHeadlessWindow() override;
		VulkanHeadlessWindow(const VulkanHeadlessWindow&) = delete;
		VulkanHeadlessWindow& operator=(const VulkanHeadlessWindow&) = delete;

		virtual void Update() override;

		virtual bool Resize(const glm::ivec2& size) override;

		virtual void NewFrame() override;

		virtual void EndFrame() override;

		virtual void Clear(const glm::vec4& color) override;

		virtual void Present(std::shared_ptr<Texture> texture) override;

		virtual void ImGuiBegin() override;

		virtual void ImGuiEnd() override;

		virtual void* BeginFrame() override;

		virtual void EndFrame(void* frame) override;

		virtual void ImGuiRenderPass() override;

		virtual void DisableCursor() override;

		virtual void ShowCursor() override;

		virtual void HideCursor() override;

		virtual void SetTitle(const std::string& title) override;
	private:

		VulkanFrameInfo m_Frame;
	};

}
