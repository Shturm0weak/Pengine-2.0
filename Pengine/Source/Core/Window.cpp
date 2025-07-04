#include "Window.h"

#include "Logger.h"

#include "../EventSystem/EventSystem.h"
#include "../EventSystem/ResizeEvent.h"

#include "../Vulkan/VulkanWindow.h"
#include "../Vulkan/VulkanHeadlessWindow.h"

using namespace Pengine;

std::shared_ptr<Window> Window::Create(const std::string& title, const std::string& name, const glm::ivec2& size)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanWindow>(title, name, size);
	}

	FATAL_ERROR("Failed to create the window, no graphics API implementation");
	return nullptr;
}

std::shared_ptr<Window> Window::CreateHeadless(const std::string& title, const std::string& name, const glm::ivec2& size)
{
	if (graphicsAPI == GraphicsAPI::Vk)
	{
		return std::make_shared<Vk::VulkanHeadlessWindow>(title, name, size);
	}

	FATAL_ERROR("Failed to create the window, no graphics API implementation");
	return nullptr;
}

Window::Window(std::string title, std::string name, const glm::ivec2& size)
	: m_Title(std::move(title))
	, m_Name(std::move(name))
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

void Window::SetEditor(bool hasEditor)
{
	if (!hasEditor)
	{
		return;
	}

	if (!m_Editor)
	{
		m_Editor = std::make_unique<Editor>();
	}
}

void Window::EditorUpdate(const std::shared_ptr<Scene>& scene)
{
	if (m_Editor)
	{
		m_Editor->Update(scene, *this);
	}
}

void Window::SetContextCurrent()
{
	ImGui::SetCurrentContext(m_ImGuiContext);
	glfwMakeContextCurrent(m_Window);
}
