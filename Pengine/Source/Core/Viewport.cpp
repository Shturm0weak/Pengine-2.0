#include "Viewport.h"

#include "FileFormatNames.h"
#include "Logger.h"
#include "SceneManager.h"
#include "Serializer.h"

#include "../Components/Camera.h"
#include "../EventSystem/EventSystem.h"
#include "../EventSystem/NextFrameEvent.h"
#include "../EventSystem/ResizeEvent.h"
#include "../Utils/Utils.h"

using namespace Pengine;

Viewport::Viewport(std::string name, const glm::ivec2& size)
	: m_Name(std::move(name))
	, m_Size(size)
{
	Resize(size);
}

void Viewport::Update(const std::shared_ptr<Texture>& viewportTexture)
{
	if (!viewportTexture)
	{
		FATAL_ERROR("Viewport texture is nullptr!");
	}

	ImGui::Begin(m_Name.c_str());

	ImGui::Image(viewportTexture->GetId(), ImVec2(m_Size.x, m_Size.y));

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSETS_BROWSER_ITEM"))
		{
			std::string path(static_cast<const char*>(payload->Data));
			path.resize(payload->DataSize);

			std::shared_ptr<Scene> scene;
			if (m_Camera)
			{
				scene = m_Camera->GetScene();
			}

			if (scene && FileFormats::Prefab() == Utils::GetFileFormat(path))
			{
				auto callback = [weakScene = std::weak_ptr<Scene>(scene), path]()
				{
					if (const std::shared_ptr<Scene> currentScene = weakScene.lock())
					{
						Serializer::DeserializePrefab(path, currentScene);
					}
				};

				NextFrameEvent* event = new NextFrameEvent(callback, Event::Type::OnNextFrame, this);
				EventSystem::GetInstance().SendEvent(event);
			}
			else if (FileFormats::Scene() == Utils::GetFileFormat(path))
			{
				auto callback = [weakScene = std::weak_ptr<Scene>(scene), path]()
				{
					if (std::shared_ptr<Scene> currentScene = weakScene.lock())
					{
						SceneManager::GetInstance().Delete(currentScene->GetName());
					}

					Serializer::DeserializeScene(path);
				};

				NextFrameEvent* event = new NextFrameEvent(callback, Event::Type::OnNextFrame, this);
				EventSystem::GetInstance().SendEvent(event);
			}
		}

		ImGui::EndDragDropTarget();
	}

	m_IsHovered = ImGui::IsWindowHovered();
	m_IsFocused = ImGui::IsWindowFocused();

	if (m_Size.x != ImGui::GetWindowSize().x || m_Size.y != ImGui::GetWindowSize().y)
	{
		Resize(glm::ivec2(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y));
	}

	m_Position = glm::vec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);

	if (m_DrawGizmosCallback)
	{
		m_DrawGizmosCallback(m_Position, m_Size, m_Camera);
		m_DrawGizmosCallback = {};
	}

	ImGui::End();
}

void Viewport::SetCamera(const std::shared_ptr<Entity>& camera)
{
	if (!camera || !camera->HasComponent<Camera>())
	{
		return;
	}

	if (m_Camera && m_Camera->HasComponent<Camera>())
	{
		m_Camera->GetComponent<Camera>().SetViewport(none);
	}

	m_Camera = camera;
	Camera& cameraComponent = m_Camera->GetComponent<Camera>();
	cameraComponent.SetViewport(m_Name);
	if (m_Size.x != cameraComponent.GetSize().x || m_Size.y != cameraComponent.GetSize().y)
	{
		cameraComponent.SetSize(m_Size);
	}
}

void Viewport::SetDrawGizmosCallback(const std::function<void(const glm::vec2& position, glm::ivec2 size, std::shared_ptr<Entity> camera)>& drawGizmosCallback)
{
	m_DrawGizmosCallback = drawGizmosCallback;
}

void Viewport::Resize(const glm::ivec2& size)
{
	m_Size = size;

	if (m_Camera)
	{
		m_Camera->GetComponent<Camera>().SetSize(m_Size);
	}
	
	ResizeEvent* event = new ResizeEvent(size, m_Name, Event::Type::OnResize, this);
	EventSystem::GetInstance().SendEvent(event);
}
