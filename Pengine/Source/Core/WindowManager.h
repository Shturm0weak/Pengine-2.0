#pragma once

#include "Core.h"
#include "Window.h"

namespace Pengine
{

	class PENGINE_API WindowManager
	{
	public:
		static WindowManager& GetInstance();

		std::shared_ptr<Window> Create(const std::string& name, const glm::ivec2& size);

		bool Destroy(std::shared_ptr<Window> window);

		std::shared_ptr<Window> GetCurrentWindow() const { return m_CurrentWindow; }

		void SetCurrentWindow(std::shared_ptr<Window> window) { m_CurrentWindow = window; }

		std::vector<std::shared_ptr<Window>> GetWindows() const { return m_Windows; }

		void ShutDown();

	private:
		WindowManager() = default;
		~WindowManager();
		WindowManager(const WindowManager&) = delete;
		WindowManager& operator=(const WindowManager&) = delete;

		std::vector<std::shared_ptr<Window>> m_Windows;
		std::shared_ptr<Window> m_CurrentWindow;
	};

}