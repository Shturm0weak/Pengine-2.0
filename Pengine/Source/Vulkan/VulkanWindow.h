#pragma once

#include "../Core/Core.h"
#include "../Core/Window.h"

#include "VulkanFrameInfo.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui/backends/imgui_impl_vulkan.h>

namespace Pengine::Vk
{

	class PENGINE_API VulkanWindow final : public Window
	{
	public:
		VulkanWindow(const std::string& title, const std::string& name, const glm::ivec2& size);
		virtual ~VulkanWindow() override;
		VulkanWindow(const VulkanWindow&) = delete;
		VulkanWindow& operator=(const VulkanWindow&) = delete;

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

		[[nodiscard]] ImGui_ImplVulkanH_Window& GetVulkanWindow() { return m_VulkanWindow; }

		[[nodiscard]] VkExtent2D GetExtent() const;

	private:
		void InitializeImGui();

		ImGui_ImplVulkanH_Window m_VulkanWindow{};
		VkSurfaceKHR m_Surface{};

		std::vector<VulkanFrameInfo> m_Frames;

		VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;
	};

}
