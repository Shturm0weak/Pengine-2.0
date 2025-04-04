#pragma once

#include "Core.h"
#include "Window.h"

namespace Pengine
{

	class PENGINE_API WindowManager
	{
	public:
		static WindowManager& GetInstance();

		WindowManager(const WindowManager&) = delete;
		WindowManager& operator=(const WindowManager&) = delete;

		std::shared_ptr<Window> Create(const std::string& title, const std::string& name, const glm::ivec2& size);

		[[nodiscard]] std::shared_ptr<Window> GetWindowByName(const std::string& name) const;

		[[nodiscard]] std::shared_ptr<Window> GetWindowByGLFW(GLFWwindow* glfwWindow) const;

		bool Destroy(const std::shared_ptr<Window>& window);

		[[nodiscard]] std::shared_ptr<Window> GetCurrentWindow() const { return m_CurrentWindow; }

		void SetCurrentWindow(const std::shared_ptr<Window>& window) { m_CurrentWindow = window; }

		[[nodiscard]] std::unordered_map<std::string, std::shared_ptr<Window>> GetWindows() const { return m_Windows; }

		void ShutDown();

	private:
		WindowManager() = default;
		~WindowManager();

		std::unordered_map<std::string, std::shared_ptr<Window>> m_Windows;
		std::unordered_map<GLFWwindow*, std::shared_ptr<Window>> m_WindowsByGLFW;
		std::shared_ptr<Window> m_CurrentWindow;
	};

}