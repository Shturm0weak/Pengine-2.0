#include "ViewportManager.h"

#include "../Utils/Utils.h"

using namespace Pengine;

ViewportManager& ViewportManager::GetInstance()
{
	static ViewportManager viewportManager;
	return viewportManager;
}

std::shared_ptr<Viewport> ViewportManager::Create(const std::string& name, const glm::ivec2& size)
{
	std::shared_ptr<Viewport> viewport = std::make_shared<Viewport>(name, size);
	m_Viewports[name] = viewport;

	return viewport;
}

std::shared_ptr<Viewport> ViewportManager::GetViewport(const std::string& name)
{
	return Utils::Find<std::shared_ptr<Viewport>>(name, m_Viewports);
}

bool ViewportManager::Destroy(std::shared_ptr<Viewport> viewport)
{
	for (auto viewportIter = m_Viewports.begin(); viewportIter != m_Viewports.end(); viewportIter++)
	{
		if (viewportIter->second == viewport)
		{
			m_Viewports.erase(viewportIter);
			return true;
		}
	}

	return false;
}

void ViewportManager::ShutDown()
{
	m_Viewports.clear();
}

ViewportManager::~ViewportManager()
{
	ShutDown();
}