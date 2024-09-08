#include "Viewport.h"

#include "FileFormatNames.h"
#include "Logger.h"
#include "SceneManager.h"
#include "Serializer.h"
#include "ViewportManager.h"

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
	UpdateProjectionMat4();

	if (!viewportTexture)
	{
		FATAL_ERROR("Viewport texture is nullptr!");
	}

	if (!m_IsOpened)
	{
		ViewportManager::GetInstance().Destroy(shared_from_this());
		return;
	}

	ImGui::Begin(m_Name.c_str(), &m_IsOpened);

	ImGui::Image(viewportTexture->GetId(), ImVec2(m_Size.x, m_Size.y));

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSETS_BROWSER_ITEM"))
		{
			std::wstring dragDropSource(static_cast<const wchar_t*>(payload->Data));
			dragDropSource.resize(payload->DataSize / sizeof(wchar_t));
			std::filesystem::path path = dragDropSource;

			std::shared_ptr<Scene> scene;
			if (const std::shared_ptr<Entity> camera = m_Camera.lock())
			{
				scene = camera->GetScene();
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
		const std::shared_ptr<Entity> camera = m_Camera.lock();
		{
			m_DrawGizmosCallback(m_Position, m_Size, camera);
		}
		m_DrawGizmosCallback = {};
	}

	ImGui::End();
}

void Viewport::SetCamera(const std::shared_ptr<Entity>& camera)
{
	std::shared_ptr<Entity> previousCamera = m_Camera.lock();
	if (previousCamera && previousCamera->HasComponent<Camera>())
	{
		Camera& cameraComponent = m_Camera.lock()->GetComponent<Camera>();
		cameraComponent.DeleteRenderTarget(m_Name);
	}

	m_Camera = camera;

	if (!camera)
	{
		return;
	}

	Camera& cameraComponent = camera->GetComponent<Camera>();
	cameraComponent.CreateRenderTarget(m_Name, m_Size);
}

void Viewport::SetDrawGizmosCallback(const std::function<void(const glm::vec2& position, glm::ivec2 size, std::shared_ptr<Entity> camera)>& drawGizmosCallback)
{
	m_DrawGizmosCallback = drawGizmosCallback;
}

void Viewport::Resize(const glm::ivec2& size)
{
	m_Size = size;

	if (const std::shared_ptr<Entity> camera = m_Camera.lock())
	{
		camera->GetComponent<Camera>().ResizeRenderTarget(m_Name, m_Size);
	}
	
	ResizeEvent* event = new ResizeEvent(size, m_Name, Event::Type::OnResize, this);
	EventSystem::GetInstance().SendEvent(event);
}

void Viewport::UpdateProjectionMat4()
{
	const std::shared_ptr<Entity> camera = m_Camera.lock();
	if (!camera || !camera->HasComponent<Camera>())
	{
		return;
	}

	const Camera& cameraComponenet = camera->GetComponent<Camera>();

	switch (cameraComponenet.GetType())
	{
	case Camera::Type::PERSPECTIVE:
	{
		SetPerspective(m_Size, cameraComponenet.GetZFar(), cameraComponenet.GetZNear(), cameraComponenet.GetFov());
		break;
	}
	case Camera::Type::ORTHOGRAPHIC:
	default:
	{
		SetOrthographic(m_Size, cameraComponenet.GetZFar(), cameraComponenet.GetZNear());
		break;
	}
	}
}

void Viewport::SetOrthographic(const glm::ivec2& size, const float zFar, const float zNear)
{
	if (size.x == 0 || size.y == 0)
	{
		m_Size = { 1, 1 };
	}
	else
	{
		m_Size = size;
	}

	const float ratio = static_cast<float>(size.x) / static_cast<float>(size.y);
	m_Projection = glm::ortho(-ratio, ratio, -1.0f,
		1.0f, zFar, zNear);
}

void Viewport::SetPerspective(const glm::ivec2& size, const float zFar, const float zNear, const float fov)
{
	if (size.x == 0 || size.y == 0)
	{
		m_Size = { 1, 1 };
	}
	else
	{
		m_Size = size;
	}

	const float ratio = static_cast<float>(size.x) / static_cast<float>(size.y);
	m_Projection = glm::perspective(fov, ratio, zNear, zFar);
}
