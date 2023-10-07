#include "Viewport.h"

#include "Camera.h"
#include "FileFormatNames.h"
#include "Logger.h"
#include "MaterialManager.h"
#include "Serializer.h"
#include "SceneManager.h"

#include "../Components/Renderer3D.h"
#include "../EventSystem/EventSystem.h"
#include "../EventSystem/NextFrameEvent.h"
#include "../EventSystem/ResizeEvent.h"
#include "../Utils/Utils.h"

using namespace Pengine;

Viewport::Viewport(const std::string& name, const glm::ivec2& size)
	: m_Name(name)
	, m_Size(size)
{
	Resize(size);
}

void Viewport::Update(std::shared_ptr<Texture> viewportTexture)
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
			std::string path((const char*)payload->Data);
			path.resize(payload->DataSize);

			if (FileFormats::Prefab() == Utils::GetFileFormat(path))
			{
				Serializer::DeserializePrefab(path, m_Camera->GetScene());
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

	ImGui::End();
}

void Viewport::SetCamera(std::shared_ptr<class Camera> camera)
{
	m_Camera = camera;
	if (m_Size.x != camera->GetSize().x || m_Size.y != camera->GetSize().y)
	{
		m_Camera->SetSize(m_Size);
	}
}

void Viewport::Resize(const glm::ivec2& size)
{
	m_Size = size;

	if (m_Camera)
	{
		m_Camera->SetSize(m_Size);
	}

	ResizeEvent* event = new ResizeEvent(size, m_Name, Event::Type::OnResize, this);
	EventSystem::GetInstance().SendEvent(event);
}
