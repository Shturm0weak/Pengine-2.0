#include "Window.h"

#include "../EventSystem/EventSystem.h"
#include "../EventSystem/ResizeEvent.h"

using namespace Pengine;

Window::Window(std::string name, const glm::ivec2& size)
	: m_Name(std::move(name))
	, m_Size(size)
{
}

bool Window::Resize(const glm::ivec2& size)
{
	m_Size = size;
	m_IsMinimized = size.x <= 0 || size.y <= 0;
	if (m_IsMinimized)
	{
		return false;
	}

	std::shared_ptr<ResizeEvent> event = std::make_shared<ResizeEvent>(size, m_Name, Event::Type::OnResize, this);
	EventSystem::GetInstance().SendEvent(event);

	return true;
}
