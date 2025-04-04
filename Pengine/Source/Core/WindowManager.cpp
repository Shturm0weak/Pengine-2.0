#include "WindowManager.h"

#include "Logger.h"

#include "../Vulkan/VulkanWindow.h"

using namespace Pengine;
using namespace Vk;

WindowManager& WindowManager::GetInstance()
{
	static WindowManager windowManager;
	return windowManager;
}

std::shared_ptr<Window> WindowManager::Create(const std::string& title, const std::string& name, const glm::ivec2& size)
{
	std::shared_ptr<Window> window = GetWindowByName(name);
	if (window)
	{
		Logger::Warning("Window with name " + name + " already exists!");
		return window;
	}

	if (graphicsAPI == GraphicsAPI::Vk)
	{
		window = std::make_shared<VulkanWindow>(title, name, size);
	}

	m_Windows.emplace(name, window);
	m_WindowsByGLFW.emplace(window->GetGLFWWindow(), window);
	return window;
}

std::shared_ptr<Window> WindowManager::GetWindowByName(const std::string& name) const
{
	auto foundWindow = m_Windows.find(name);
	if (foundWindow != m_Windows.end())
	{
		return foundWindow->second;
	}

	return nullptr;
}

std::shared_ptr<Window> WindowManager::GetWindowByGLFW(GLFWwindow* glfwWindow) const
{
	auto foundWindow = m_WindowsByGLFW.find(glfwWindow);
	if (foundWindow != m_WindowsByGLFW.end())
	{
		return foundWindow->second;
	}

	return nullptr;
}

bool WindowManager::Destroy(const std::shared_ptr<Window>& window)
{
	auto foundWindow = m_Windows.find(window->GetName());
	if (foundWindow != m_Windows.end())
	{
		m_WindowsByGLFW.erase(foundWindow->second->GetGLFWWindow());
		m_Windows.erase(foundWindow);
		return true;
	}

	return false;
}

void WindowManager::ShutDown()
{
	m_Windows.clear();
	m_WindowsByGLFW.clear();
	m_CurrentWindow = nullptr;
}

WindowManager::~WindowManager()
{
	ShutDown();
}
