#pragma once

#include "../Core/Core.h"
#include "../Core/Window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui/backends/imgui_impl_vulkan.h>

namespace Pengine
{

	namespace Vk
	{

		class PENGINE_API VulkanWindow : public Window
		{
		public:
			VulkanWindow(const std::string& name, const glm::ivec2& size);
			~VulkanWindow();
			VulkanWindow(const VulkanWindow&) = delete;
			VulkanWindow& operator=(const VulkanWindow&) = delete;

			virtual void Update() override;

			virtual bool Resize(const glm::ivec2& size) override;

			virtual void NewFrame() override;

			virtual void EndFrame() override;

			virtual void Clear(const glm::vec4& color) override;

			virtual void Present(std::shared_ptr<Texture> texture) override;

			virtual void ShutDownPrepare() override;

			virtual void ImGuiBegin() override;

			virtual void ImGuiEnd() override;

			virtual void* BeginFrame() override;

			virtual void EndFrame(void* frame) override;

			virtual void ImGuiRenderPass(void* frame) override;

			virtual void DisableCursor() override;

			virtual void ShowCursor() override;

			virtual void HideCursor() override;

			GLFWwindow* GetRawWindow() { return m_Window; }

			ImGui_ImplVulkanH_Window& GetVulkanWindow() { return m_VulkanWindow; }

			VkExtent2D GetExtent() const;

		private:
			void InitializeImGui();

			GLFWwindow* m_Window = nullptr;
			ImGui_ImplVulkanH_Window m_VulkanWindow;
			VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
		};

	}

}