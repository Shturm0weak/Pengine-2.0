#pragma once

#include "Core.h"
#include "ViewportManager.h"

#include "../Graphics/Texture.h"

#include "GLFW/glfw3.h"

namespace Pengine
{
	class Viewport;

	class PENGINE_API Window
	{
	public:
		static std::shared_ptr<Window> Create(const std::string& title, const std::string& name, const glm::ivec2& size);

		static std::shared_ptr<Window> CreateHeadless(const std::string& title, const std::string& name, const glm::ivec2& size);

		Window(std::string title, std::string name, const glm::ivec2& size);
		virtual ~Window() = default;
		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		virtual void Update() = 0;

		virtual bool Resize(const glm::ivec2& size);

		virtual void NewFrame() = 0;

		virtual void EndFrame() = 0;

		virtual void Clear(const glm::vec4& color) = 0;

		virtual void Present(std::shared_ptr<Texture> texture) = 0;

		virtual void ImGuiBegin() = 0;

		virtual void ImGuiEnd() = 0;

		virtual void* BeginFrame() = 0;

		virtual void EndFrame(void* frame) = 0;

		virtual void ImGuiRenderPass() = 0;

		virtual void DisableCursor() = 0;

		virtual void ShowCursor() = 0;

		virtual void HideCursor() = 0;

		virtual void SetTitle(const std::string& title) = 0;

		void SetIsRunning(const bool isRunning) { m_IsRunning = isRunning; }

		void SetContextCurrent();

		[[nodiscard]] GLFWwindow* GetGLFWWindow() const { return m_Window; }

		[[nodiscard]] const std::string& GetName() const { return m_Name; }

		[[nodiscard]] const std::string& GetTitle() const { return m_Title; }

		[[nodiscard]] bool IsRunning() const { return m_IsRunning; }

		[[nodiscard]] bool IsMinimized() const { return m_IsMinimized; }

		[[nodiscard]] glm::ivec2 GetSize() const { return m_Size; }

		[[nodiscard]] ImGuiContext* GetImGuiContext() const { return m_ImGuiContext; }
		
		[[nodiscard]] ViewportManager& GetViewportManager() { return m_ViewportManager; }

		[[nodiscard]] bool IsHeadless() const { return m_IsHeadless; }

	protected:
		std::string m_Name;
		std::string m_Title;
		glm::ivec2 m_Size = { 0, 0 };

		bool m_IsRunning = true;
		bool m_IsMinimized = false;
		bool m_HasEditor = false;
		bool m_IsHeadless = false;

		GLFWwindow* m_Window = nullptr;
		ImGuiContext* m_ImGuiContext = nullptr;

		ViewportManager m_ViewportManager;
	};

}
