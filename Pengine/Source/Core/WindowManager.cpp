#include "WindowManager.h"

#include "../Vulkan/VulkanWindow.h"

using namespace Pengine;
using namespace Vk;

WindowManager& WindowManager::GetInstance()
{
	static WindowManager windowManager;
	return windowManager;
}

std::shared_ptr<Window> WindowManager::Create(const std::string& name, const glm::ivec2& size)
{
	std::shared_ptr<Window> window = nullptr;
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		window = std::make_shared<VulkanWindow>(name, size);
	}

	m_Windows.emplace_back(window);
	return window;
}

bool WindowManager::Destroy(const std::shared_ptr<Window>& window)
{
	for (auto windowIter = m_Windows.begin(); windowIter != m_Windows.end(); ++windowIter)
	{
		if (*windowIter == window)
		{
			m_Windows.erase(windowIter);
			return true;
		}
	}

	return false;
}

void WindowManager::ShutDown()
{
	m_Windows.clear();
	m_CurrentWindow = nullptr;
}

WindowManager::~WindowManager()
{
	ShutDown();
}