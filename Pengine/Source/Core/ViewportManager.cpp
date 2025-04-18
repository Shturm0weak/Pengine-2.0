#include "ViewportManager.h"

#include "Viewport.h"

#include "../Utils/Utils.h"

using namespace Pengine;

std::shared_ptr<Viewport> ViewportManager::Create(const std::string& name, const glm::ivec2& size)
{
	if (const auto viewport = GetViewport(name))
	{
		return viewport;
	}

	std::shared_ptr<Viewport> viewport = std::make_shared<Viewport>(name, size);
	m_Viewports[name] = viewport;

	return viewport;
}

std::shared_ptr<Viewport> ViewportManager::GetViewport(const std::string& name) const
{
	return Utils::Find<std::shared_ptr<Viewport>>(name, m_Viewports);
}

bool ViewportManager::Destroy(const std::shared_ptr<Viewport>& viewport)
{
	for (auto viewportIter = m_Viewports.begin(); viewportIter != m_Viewports.end(); ++viewportIter)
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
