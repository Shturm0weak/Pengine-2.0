#include "Editor.h"

#include "../Components/Camera.h"
#include "../Components/DirectionalLight.h"
#include "../Components/PointLight.h"
#include "../Components/Renderer3D.h"
#include "../Components/Transform.h"
#include "../Core/FileFormatNames.h"
#include "../Core/Input.h"
#include "../Core/KeyCode.h"
#include "../Core/SceneManager.h"
#include "../Core/MaterialManager.h"
#include "../Core/MeshManager.h"
#include "../Core/Serializer.h"
#include "../Core/TextureManager.h"
#include "../Core/Time.h"
#include "../Core/ViewportManager.h"
#include "../Core/Viewport.h"
#include "../Core/WindowManager.h"
#include "../Core/RenderPassManager.h"
#include "../Core/RenderPassOrder.h"
#include "../Editor/ImGuizmo.h"
#include "../EventSystem/EventSystem.h"
#include "../EventSystem/NextFrameEvent.h"

#include <fstream>
#include <format>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace Pengine;

Editor::Editor()
{
	SetDarkThemeColors();
	ViewportManager::GetInstance().Create("Main", { 800, 800 });
}

void Editor::Update(const std::shared_ptr<Scene>& scene)
{
	if (Input::Mouse::IsMouseReleased(Keycode::MOUSE_BUTTON_2))
	{
		m_MovingCamera = nullptr;
	}

	for (const auto& [name, viewport] : ViewportManager::GetInstance().GetViewports())
	{
		if (viewport->IsHovered() && Input::Mouse::IsMouseDown(Keycode::MOUSE_BUTTON_2))
		{
			m_MovingCamera = viewport->GetCamera().lock();
		}

		if (m_MovingCamera)
		{
			WindowManager::GetInstance().GetCurrentWindow()->DisableCursor();
			MoveCamera(m_MovingCamera);
			break;
		}
		else
		{
			WindowManager::GetInstance().GetCurrentWindow()->ShowCursor();
		}
	}

	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_LEFT_CONTROL) && Input::KeyBoard::IsKeyPressed(Keycode::KEY_F))
	{
		m_FullScreen = !m_FullScreen;
	}

	if (m_FullScreen)
	{
		return;
	}

	Manipulate(scene);

	MainMenuBar();
	Hierarchy(scene);
	SceneInfo(scene);
	Properties(scene);
	AssetBrowser(scene);

	m_MaterialMenu.Update(*this);
	m_BaseMaterialMenu.Update(*this);
	m_CreateFileMenu.Update();
	m_DeleteFileMenu.Update();
	m_CloneMaterialMenu.Update();
	m_CreateViewportMenu.Update(*this);

	ImGui::Begin("Settings");
	ImGui::Text("FPS: %.0f", 1.0f / static_cast<float>(Time::GetDeltaTime()));
	ImGui::Text("DrawCalls: %d", drawCallsCount);
	ImGui::Text("Triangles: %d", static_cast<int>(vertexCount));
	ImGui::Text("Meshes: %d", static_cast<int>(MeshManager::GetInstance().GetMeshes().size()));
	ImGui::Text("BaseMaterials: %d", static_cast<int>(MaterialManager::GetInstance().GetBaseMaterials().size()));
	ImGui::Text("Materials: %d", static_cast<int>(MaterialManager::GetInstance().GetMaterials().size()));
	ImGui::Text("Textures: %d", static_cast<int>(TextureManager::GetInstance().GetTextures().size()));

	ImGui::End();
}

bool Editor::DrawVec2Control(
	const std::string& label,
	glm::vec2& values,
	const float resetValue,
	const glm::vec2& limits,
	const float speed,
	const float columnWidth) const
{
	bool changed = false;

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	if (m_DrawVecLabel) ImGui::Text(label.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 5.0f });

	const float lineHeight = ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f;
	const ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	if (ImGui::Button("X", buttonSize))
	{
		values.x = resetValue;
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragFloat("##X", &values.x, speed, limits.x, limits.y, "%.2f"))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	if (ImGui::Button("Y", buttonSize))
	{
		values.y = resetValue;
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragFloat("##Y", &values.y, speed, limits.x, limits.y, "%.2f"))
	{
		changed = true;
	}
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();

	return changed;
}

bool Editor::DrawVec3Control(
	const std::string& label,
	glm::vec3& values,
	const float resetValue,
	const glm::vec2& limits,
	const float speed,
	const float columnWidth) const
{
	bool changed = false;

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	if (m_DrawVecLabel) ImGui::Text(label.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 5.0f });

	const float lineHeight = ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f;
	const ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	if (ImGui::Button("X", buttonSize))
	{
		values.x = resetValue;
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragFloat("##X", &values.x, speed, limits.x, limits.y, "%.2f"))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	if (ImGui::Button("Y", buttonSize))
	{
		values.y = resetValue;
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragFloat("##Y", &values.y, speed, limits.x, limits.y, "%.2f"))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	if (ImGui::Button("Z", buttonSize))
	{
		values.z = resetValue;
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragFloat("##Z", &values.z, speed, limits.x, limits.y, "%.2f"))
	{
		changed = true;
	}
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();

	return changed;
}

bool Editor::DrawVec4Control(
	const std::string& label,
	glm::vec4& values,
	const float resetValue,
	const glm::vec2& limits,
	const float speed,
	const float columnWidth) const
{
	bool changed = false;

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	if (m_DrawVecLabel) ImGui::Text(label.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(4, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 5.0f });

	const float lineHeight = ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f;
	const ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	if (ImGui::Button("X", buttonSize))
	{
		values.x = resetValue;
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragFloat("##X", &values.x, speed, limits.x, limits.y, "%.2f"))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	if (ImGui::Button("Y", buttonSize))
	{
		values.y = resetValue;
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragFloat("##Y", &values.y, speed, limits.x, limits.y, "%.2f"))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	if (ImGui::Button("Z", buttonSize))
	{
		values.z = resetValue;
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragFloat("##Z", &values.z, speed, limits.x, limits.y, "%.2f"))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.7f, 0.7f, 0.7f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.9f, 0.9f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.7f, 0.7f, 0.7f, 1.0f });
	if (ImGui::Button("W", buttonSize))
	{
		values.w = resetValue;
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragFloat("##W", &values.w, speed, limits.x, limits.y, "%.2f"))
	{
		changed = true;
	}
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();

	return changed;
}

bool Editor::DrawIVec2Control(
	const std::string& label,
	glm::ivec2& values,
	const float resetValue,
	const glm::vec2& limits,
	const float speed,
	const float columnWidth) const
{
	bool changed = false;

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	if (m_DrawVecLabel) ImGui::Text(label.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 5.0f });

	const float lineHeight = ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f;
	const ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	if (ImGui::Button("X", buttonSize))
	{
		values.x = static_cast<int>(resetValue);
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragInt("##X", &values.x, speed, limits.x, limits.y))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	if (ImGui::Button("Y", buttonSize))
	{
		values.y = static_cast<int>(resetValue);
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragInt("##Y", &values.y, speed, limits.x, limits.y))
	{
		changed = true;
	}
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();

	return changed;
}

bool Editor::DrawIVec3Control(
	const std::string& label,
	glm::ivec3& values,
	const float resetValue,
	const glm::vec2& limits,
	const float speed,
	const float columnWidth) const
{
	bool changed = false;

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	if (m_DrawVecLabel) ImGui::Text(label.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 5.0f });

	const float lineHeight = ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f;
	const ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	if (ImGui::Button("X", buttonSize))
	{
		values.x = static_cast<int>(resetValue);
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragInt("##X", &values.x, speed, limits.x, limits.y))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	if (ImGui::Button("Y", buttonSize))
	{
		values.y = static_cast<int>(resetValue);
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragInt("##Y", &values.y, speed, limits.x, limits.y))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	if (ImGui::Button("Z", buttonSize))
	{
		values.z = static_cast<int>(resetValue);
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragInt("##Z", &values.z, speed, limits.x, limits.y))
	{
		changed = true;
	}
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();

	return changed;
}

bool Editor::DrawIVec4Control(
	const std::string& label,
	glm::ivec4& values,
	const float resetValue,
	const glm::vec2& limits,
	const float speed,
	const float columnWidth) const
{
	bool changed = false;

	ImGuiIO& io = ImGui::GetIO();

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	if (m_DrawVecLabel) ImGui::Text(label.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(4, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 5.0f });

	const float lineHeight = ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f;
	const ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	if (ImGui::Button("X", buttonSize))
	{
		values.x = static_cast<int>(resetValue);
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragInt("##X", &values.x, speed, limits.x, limits.y))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	if (ImGui::Button("Y", buttonSize))
	{
		values.y = static_cast<int>(resetValue);
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragInt("##Y", &values.y, speed, limits.x, limits.y))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	if (ImGui::Button("Z", buttonSize))
	{
		values.z = static_cast<int>(resetValue);
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragInt("##Z", &values.z, speed, limits.x, limits.y))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.7f, 0.7f, 0.7f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.9f, 0.9f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.7f, 0.7f, 0.7f, 1.0f });
	if (ImGui::Button("W", buttonSize))
	{
		values.w = static_cast<int>(resetValue);
		changed = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragInt("##W", &values.w, speed, limits.x, limits.y))
	{
		changed = true;
	}
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();

	return changed;
}

void Editor::Hierarchy(const std::shared_ptr<Scene>& scene)
{
	if (ImGui::Begin("Hierarchy"))
	{
		if (scene)
		{
			if (ImGui::CollapsingHeader("Game Objects"))
			{
				DrawScene(scene);
			}

			GameObjectPopUpMenu(scene);
		}

		ImGui::End();
	}
}

void Editor::SceneInfo(const std::shared_ptr<Scene>& scene)
{
	if (ImGui::Begin("Scene"))
	{
		if (scene)
		{
			ImGui::Text("Name: %s", scene->GetName().c_str());
			ImGui::Text("Filepath: %s", scene->GetFilepath().string().c_str());
			ImGui::Text("Tag: %s", scene->GetTag().c_str());
			ImGui::Text("Entities Count: %u", scene->GetEntities().size());

			bool drawBoundingBoxes = scene->GetSettings().m_DrawBoundingBoxes;
			if (ImGui::Checkbox("Draw Bounding Boxes", &drawBoundingBoxes))
			{
				scene->GetSettings().m_DrawBoundingBoxes = drawBoundingBoxes;
			}

			GraphicsSettingsInfo(scene->GetGraphicsSettings());
		}

		ImGui::End();
	}
}

void Editor::DrawScene(const std::shared_ptr<Scene>& scene)
{
	Indent indent;

	if (ImGui::TreeNodeEx((void*)&scene, 0, scene->GetName().c_str()))
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT"))
			{
				std::string uuid((const char*)payload->Data);
				uuid.resize(payload->DataSize);
				auto callback = [weakScene = std::weak_ptr<Scene>(scene), uuid]()
				{
					if (const std::shared_ptr<Scene> currentScene = weakScene.lock())
					{
						if (const std::shared_ptr<Entity> entity = currentScene->FindEntityByUUID(uuid); entity)
						{
							if (const std::shared_ptr<Entity> parent = entity->GetParent())
							{
								parent->RemoveChild(entity);
							}
						}
					}
				};

				std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
				EventSystem::GetInstance().SendEvent(event);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::TreePop();

		const std::vector<std::shared_ptr<Entity>>& entities = scene->GetEntities();
		for (const std::shared_ptr<Entity>& entity : entities)
		{
			if (entity->HasParent())
			{
				continue;
			}

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
			flags |= scene->GetSelectedEntities().count(entity) ?
				ImGuiTreeNodeFlags_Selected : 0;
			
			DrawNode(entity, flags);
		}
	}
}

void Editor::DrawNode(const std::shared_ptr<Entity>& entity, ImGuiTreeNodeFlags flags)
{
	flags |= entity->GetChilds().empty() ? ImGuiTreeNodeFlags_Leaf : 0;

	Indent indent;

	ImGui::PushID(entity.get());
	bool enabled = entity->IsEnabled();
	if (ImGui::Checkbox("##IsEnabled", &enabled))
	{
		entity->SetEnabled(enabled);
	}
	ImGui::PopID();

	ImGui::SameLine();

	ImGuiStyle* style = &ImGui::GetStyle();

	const bool opened = ImGui::TreeNodeEx((void*)entity.get(), flags, entity->GetName().c_str());
	style->Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

	if (ImGui::BeginDragDropSource())
	{
		ImGui::SetDragDropPayload("GAMEOBJECT", (const void*)entity->GetUUID().Get().c_str(), entity->GetUUID().Get().size());
		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT"))
		{
			std::string uuid((const char*)payload->Data);
			uuid.resize(payload->DataSize);
			auto callback = [weakEntity = std::weak_ptr<Entity>(entity), uuid]()
			{
				if (const std::shared_ptr<Entity>& currentEntity = weakEntity.lock())
				{
					const std::shared_ptr<Entity>& child = currentEntity->GetScene()->FindEntityByUUID(uuid);
					if (child)
					{
						if (currentEntity->HasAsParent(child, true) ||
							(currentEntity->HasParent() && currentEntity->GetParent() == child))
						{
							return;
						}

						if (const std::shared_ptr<Entity>& parent = child->GetParent())
						{
							parent->RemoveChild(child);
						}

						currentEntity->AddChild(child);
					}
				}
			};

			std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
			EventSystem::GetInstance().SendEvent(event);
		}
		ImGui::EndDragDropTarget();
	}

	if (ImGui::IsItemHovered() && Input::Mouse::IsMouseReleased(Keycode::MOUSE_BUTTON_1))
	{
		if (!Input::KeyBoard::IsKeyDown(Keycode::KEY_LEFT_CONTROL))
		{
			entity->GetScene()->GetSelectedEntities().clear();
		}
		
		entity->GetScene()->GetSelectedEntities().emplace(entity);
	}

	if (opened)
	{
		ImGui::TreePop();
		DrawChilds(entity);
	}
}

void Editor::DrawChilds(const std::shared_ptr<Entity>& entity)
{
	for (const std::weak_ptr<Entity> weakChild : entity->GetChilds())
	{
		if (const std::shared_ptr<Entity> child = weakChild.lock())
		{
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
			flags |= entity->GetScene()->GetSelectedEntities().count(child) ?
				ImGuiTreeNodeFlags_Selected : 0;

			DrawNode(child, flags);
		}
	}
}

void Editor::Properties(const std::shared_ptr<Scene>& scene)
{
	if (ImGui::Begin("Properties", nullptr))
	{
		if (!scene)
		{
			ImGui::End();
			return;
		}

		for (const std::shared_ptr<Entity> entity : scene->GetSelectedEntities())
		{
			if (entity->HasParent())
			{
				ImGui::Text("Owner: %s", entity->GetParent()->GetName().c_str());
			}
			else
			{
				ImGui::Text("Owner: Null");
			}

			ImGui::Text("UUID: %s", entity->GetUUID().Get().c_str());

			char name[64];
			strcpy(name, entity->GetName().c_str());
			if (ImGui::InputText("Name", name, sizeof(name)))
			{
				entity->SetName(name);
			}

			ComponentsPopUpMenu(entity);

			TransformComponent(entity);
			CameraComponent(entity);
			Renderer3DComponent(entity);
			PointLightComponent(entity);
			DirectionalLightComponent(entity);

			ImGui::NewLine();
		}
		ImGui::End();
	}
}

void Editor::GraphicsSettingsInfo(GraphicsSettings& graphicsSettings)
{
	if (ImGui::CollapsingHeader("Graphics settings"))
	{
		Indent indent;

		ImGui::Text("Filepath: %s", graphicsSettings.GetFilepath().string().c_str());

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSETS_BROWSER_ITEM"))
			{
				std::wstring filepath((const wchar_t*)payload->Data);
				filepath.resize(payload->DataSize / sizeof(wchar_t));
				graphicsSettings = Serializer::DeserializeGraphicsSettings(filepath);
			}
			ImGui::EndDragDropTarget();
		}

		bool isChangedToSerialize = false;
		if (ImGui::CollapsingHeader("SSAO"))
		{
			ImGui::PushID("SSAO Is Enabled");
			isChangedToSerialize += ImGui::Checkbox("Is Enabled", &graphicsSettings.ssao.isEnabled);
			ImGui::PopID();

			const char* const resolutionScales[] = { "0.25", "0.5", "0.75", "1.0"};
			isChangedToSerialize += ImGui::Combo("Quality", &graphicsSettings.ssao.resolutionScale, resolutionScales, 4);

			isChangedToSerialize += ImGui::SliderFloat("Bias", &graphicsSettings.ssao.bias, 0.0f, 10.0f);
			isChangedToSerialize += ImGui::SliderFloat("Radius", &graphicsSettings.ssao.radius, 0.0f, 1.0f);
			isChangedToSerialize += ImGui::SliderInt("Kernel Size", &graphicsSettings.ssao.kernelSize, 2, 64);
			isChangedToSerialize += ImGui::SliderInt("Noise Size", &graphicsSettings.ssao.noiseSize, 4, 64);
			isChangedToSerialize += ImGui::SliderFloat("AO Scale", &graphicsSettings.ssao.aoScale, 0.0f, 10.0f);
		}

		if (ImGui::CollapsingHeader("Shadows"))
		{
			ImGui::PushID("Shadows Is Enabled");
			isChangedToSerialize += ImGui::Checkbox("Is Enabled", &graphicsSettings.shadows.isEnabled);
			ImGui::PopID();

			const char* const qualities[] = { "1024", "2048", "4096" };
			isChangedToSerialize += ImGui::Combo("Quality", &graphicsSettings.shadows.quality, qualities, 3);

			if (ImGui::SliderInt("Cascade Count", &graphicsSettings.shadows.cascadeCount, 2, 10))
			{
				isChangedToSerialize += true;

				graphicsSettings.shadows.biases.resize(graphicsSettings.shadows.cascadeCount, 0.0f);
			}

			ImGui::Checkbox("Visualize", &graphicsSettings.shadows.visualize);
			isChangedToSerialize += ImGui::Checkbox("Pcf Enabled", &graphicsSettings.shadows.pcfEnabled);
			isChangedToSerialize += ImGui::SliderInt("Pcf Range", &graphicsSettings.shadows.pcfRange, 1, 5);
			isChangedToSerialize += ImGui::SliderFloat("Split Factor", &graphicsSettings.shadows.splitFactor, 0.0f, 1.0f);
			isChangedToSerialize += ImGui::SliderFloat("Max Distance", &graphicsSettings.shadows.maxDistance, 0.0f, 1000.0f);
			isChangedToSerialize += ImGui::SliderFloat("Fog Factor", &graphicsSettings.shadows.fogFactor, 0.0f, 1.0f);
			for (size_t i = 0; i < graphicsSettings.shadows.biases.size(); i++)
			{
				const std::string biasName = "Bias " + std::to_string(i);
				isChangedToSerialize += ImGui::SliderFloat(biasName.c_str(), &graphicsSettings.shadows.biases[i], 0.0f, 1.0f);
			}
		}

		if (ImGui::CollapsingHeader("Bloom"))
		{
			ImGui::PushID("Bloom Is Enabled");
			isChangedToSerialize += ImGui::Checkbox("Is Enabled", &graphicsSettings.bloom.isEnabled);
			ImGui::PopID();

			isChangedToSerialize += ImGui::SliderInt("Mip Count", &graphicsSettings.bloom.mipCount, 1, 10);
			isChangedToSerialize += ImGui::SliderFloat("Brightness Threshold", &graphicsSettings.bloom.brightnessThreshold, 0.0f, 2.0f);
		}

		if (ImGui::CollapsingHeader("Post Process"))
		{
			isChangedToSerialize += ImGui::Checkbox("FXAA", &graphicsSettings.postProcess.fxaa);
			isChangedToSerialize += ImGui::SliderFloat("Gamma", &graphicsSettings.postProcess.gamma, 0.0f, 3.0f);

			const char* const toneMappers[] = {"NONE", "ACES" };
			int* toneMapperIndex = (int*)(&graphicsSettings.postProcess.toneMapper);
			isChangedToSerialize += ImGui::Combo("Tone Mapper", toneMapperIndex, toneMappers, (int)GraphicsSettings::PostProcess::ToneMapper::COUNT);
		}

		if (isChangedToSerialize && std::filesystem::exists(graphicsSettings.GetFilepath()))
		{
			Serializer::SerializeGraphicsSettings(graphicsSettings);
		}
	}
}

void Editor::CameraComponent(const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<Camera>())
	{
		return;
	}

	Camera& camera = entity->GetComponent<Camera>();
	
	if (ImGui::Button("X"))
	{
		entity->RemoveComponent<Camera>();
	}
	ImGui::SameLine();
	if (ImGui::CollapsingHeader("Camera"))
	{
		Indent indent;

		int type = static_cast<int>(camera.GetType());
		const char* types[] = { "ORTHOGRAPHIC", "PERSPECTIVE" };
		if (ImGui::Combo("Camera type", &type, types, 2))
		{
			camera.SetType(Camera::Type(type));
		}

		float fov = camera.GetFov();
		if (ImGui::SliderAngle("FOV", &fov, 0.0f, 120.0f))
		{
			camera.SetFov(fov);
		}

		float zFar = camera.GetZFar();
		if (ImGui::SliderFloat("Z Far", &zFar, 0.0f, 1000.0f))
		{
			camera.SetZFar(zFar);
		}

		float zNear = camera.GetZNear();
		if (ImGui::SliderFloat("Z Near", &zNear, 0.0f, 10.0f))
		{
			camera.SetZNear(zNear);
		}

		if (ImGui::BeginMenu("Viewports"))
		{
			for (const auto& viewport : ViewportManager::GetInstance().GetViewports())
			{
				if (ImGui::MenuItem(viewport.first.c_str()))
				{
					auto callback = [viewport, entity]()
					{
						viewport.second->SetCamera(entity);
					};

					std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
					EventSystem::GetInstance().SendEvent(event);
				}
			}
			
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Render Targets"))
		{
			for (const auto& renderPassName : renderPassPerViewportOrder)
			{
				if (ImGui::MenuItem(renderPassName.c_str()))
				{
					camera.SetRenderPassName(renderPassName);
					camera.SetRenderTargetIndex(0);
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Render Target Index"))
		{
			if (!camera.GetRenderPassName().empty())
			{
				const int indexCount = RenderPassManager::GetInstance().GetRenderPass(camera.GetRenderPassName())->GetAttachmentDescriptions().size();
				for (size_t i = 0; i < indexCount; i++)
				{
					if (ImGui::MenuItem(std::to_string(i).c_str()))
					{
						camera.SetRenderTargetIndex(i);
					}
				}
			}

			ImGui::EndMenu();
		}
	}
}

void Editor::TransformComponent(const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<Transform>())
	{
		return;
	}

	Transform& transform = entity->GetComponent<Transform>();

	if (ImGui::Button("O"))
	{
	}
	ImGui::SameLine();
	if (ImGui::CollapsingHeader("Transform"))
	{
		Indent indent;

		ImGui::Text("Entity: %s", transform.GetEntity()->GetName().c_str());

		bool followOwner = transform.GetFollorOwner();
		if (ImGui::Checkbox("Follow owner", &followOwner))
		{
			transform.SetFollowOwner(followOwner);
		}

		const char* types[] = { "Local", "Global" };
		ImGui::Combo("System", &m_TransformSystem, types, 2);

		std::shared_ptr<Entity> parent;
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
		if (m_TransformSystem == 0)
		{
			position = transform.GetPosition(Transform::System::LOCAL);
			rotation = glm::degrees(transform.GetRotation(Transform::System::LOCAL));
			scale = transform.GetScale(Transform::System::LOCAL);
		}
		else if (m_TransformSystem == 1)
		{
			if (parent = entity->GetParent())
			{
				parent->RemoveChild(entity);
			}

			position = transform.GetPosition(Transform::System::GLOBAL);
			rotation = glm::degrees(transform.GetRotation(Transform::System::GLOBAL));
			scale = transform.GetScale(Transform::System::GLOBAL);
		}

		DrawVec3Control("Translation", position);
		DrawVec3Control("Rotation", rotation, 0.0f, { -360.0f, 360.0f }, 1.0f);
		DrawVec3Control("Scale", scale, 1.0f, { 0.0f, 25.0f });

		transform.Scale(scale);
		transform.Translate(position);
		transform.Rotate(glm::radians(rotation));

		if (parent)
		{
			parent->AddChild(entity);
		}
	}
}

void Editor::GameObjectPopUpMenu(const std::shared_ptr<Scene>& scene)
{
	if (ImGui::BeginPopupContextWindow())
	{
		if (ImGui::MenuItem("Create Gameobject"))
		{
			std::shared_ptr<Entity> entity = scene->CreateEntity();
			entity->AddComponent<Transform>(entity);
		}

		if (ImGui::MenuItem("Clone Gameobject"))
		{
			if (!scene->GetSelectedEntities().empty())
			{
				if (std::shared_ptr<Entity> entity = *scene->GetSelectedEntities().rbegin())
				{
					scene->CloneEntity(entity);
				}
			}
		}

		if (!scene->GetSelectedEntities().empty())
		{
			if (ImGui::MenuItem("Delete Selected"))
			{
				auto callback = [weakScene = std::weak_ptr<Scene>(scene)]()
				{
					if (const std::shared_ptr<Scene>& currentScene = weakScene.lock())
					{
						for (std::shared_ptr<Entity> entity : currentScene->GetSelectedEntities())
						{
							if (entity)
							{
								currentScene->DeleteEntity(entity);
							}
						}
						currentScene->GetSelectedEntities().clear();
					}
				};

				std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
				EventSystem::GetInstance().SendEvent(event);
			}
		}

		if (ImGui::MenuItem("Save Scene"))
		{
			std::string sceneFilepath = scene->GetFilepath().string();
			if (sceneFilepath == none)
			{
				sceneFilepath = "Scenes/" + scene->GetName() + FileFormats::Scene();
			}
			Serializer::SerializeScene(sceneFilepath, scene);
		}
		
		ImGui::EndPopup();
	}
}

void Editor::AssetBrowser(const std::shared_ptr<Scene>& scene)
{
	if (ImGui::Begin("Asset browser"))
	{
		if (m_CurrentDirectory != m_RootDirectory)
		{
			if (ImGui::Button("<-"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}

			ImGui::SameLine();
		}

		//ImGui::InputText("Filter", m_FilterBuffer, 64);

		const float padding = 16.0f;
		const float thumbnailSize = 128.0f * m_ThumbnailScale;
		const float cellSize = padding + thumbnailSize;
		const float panelWidth = ImGui::GetContentRegionAvail().x;

		const int columns = (int)(panelWidth / cellSize);

		if (columns == 0)
		{
			ImGui::End();
			return;
		}

		ImGui::Columns(columns, 0, false);

		const bool leftMouseButtonDoubleClicked = ImGui::IsMouseDoubleClicked(GLFW_MOUSE_BUTTON_1);

		const ImTextureID folderIconId = (ImTextureID)TextureManager::GetInstance().GetTexture("Editor\\Images\\FolderIcon.png")->GetId();
		const ImTextureID fileIconId = (ImTextureID)TextureManager::GetInstance().GetTexture("Editor\\Images\\FileIcon.png")->GetId();
		const ImTextureID metaIconId = (ImTextureID)TextureManager::GetInstance().GetTexture("Editor\\Images\\MetaIcon.png")->GetId();
		const ImTextureID materialIconId = (ImTextureID)TextureManager::GetInstance().GetTexture("Editor\\Images\\MaterialIcon.png")->GetId();
		const ImTextureID meshIconId = (ImTextureID)TextureManager::GetInstance().GetTexture("Editor\\Images\\MeshIcon.png")->GetId();

		bool iconHovered = false;

		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::MenuItem("Create Scene"))
			{
				m_CreateFileMenu.opened = true;
				m_CreateFileMenu.filepath = m_CurrentDirectory;
				m_CreateFileMenu.format = FileFormats::Scene();
				m_CreateFileMenu.name[0] = '\0';
			}

			ImGui::EndPopup();
		}

		for (auto& directoryIter : std::filesystem::directory_iterator(m_CurrentDirectory))
		{
			const std::filesystem::path path = Utils::GetShortFilepath(directoryIter.path());
			if (Utils::Contains(path.string(), ".cpp") || Utils::Contains(path.string(), ".h"))
			{
				continue;
			}

			const std::string filename = path.filename().string();
			const std::string format = Utils::GetFileFormat(path);
			ImTextureID currentIcon;

			if (FileFormats::IsAsset(format))
			{
				std::filesystem::path metaFilePath = path;
				metaFilePath.concat(FileFormats::Meta());
				if (!std::filesystem::exists(metaFilePath))
				{
					Serializer::GenerateFileUUID(path);
				}
			}

			//if (m_FilterBuffer[0] != '\0' && !Utils::Contains(filename, m_FilterBuffer))
			//{
			//	continue;
			//}

			if (directoryIter.is_directory())
			{
				currentIcon = folderIconId;
			}
			else if (format == FileFormats::Mat())
			{
				currentIcon = materialIconId;
			}
			else if (format == FileFormats::Meta())
			{
				currentIcon = metaIconId;
			}
			else if (format == FileFormats::Mesh())
			{
				currentIcon = meshIconId;
			}
			else if (FileFormats::IsTexture(format))
			{
				if (const std::shared_ptr<Texture>& texture = TextureManager::GetInstance().GetTexture(path))
				{
					currentIcon = (ImTextureID)texture->GetId();
				}
				else
				{
					currentIcon = fileIconId;
				}
			}
			else
			{
				currentIcon = fileIconId;
			}

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			ImGui::PushID(filename.c_str());
			if (ImGui::ImageButton(currentIcon, { thumbnailSize, thumbnailSize }, ImVec2(0, 1), ImVec2(1, 0)))
			{
				if (format == FileFormats::Mat())
				{
					m_MaterialMenu.opened = true;
					m_MaterialMenu.material = MaterialManager::GetInstance().LoadMaterial(path);
				}
				else if (format == FileFormats::BaseMat())
				{
					m_BaseMaterialMenu.opened = true;
					m_BaseMaterialMenu.baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(path);
				}
			}
			ImGui::PopID();

			iconHovered = iconHovered ? iconHovered : ImGui::IsItemHovered();

			if (ImGui::BeginDragDropSource())
			{
				std::wstring dragDropSource = path.wstring();
				const wchar_t* assetPath = dragDropSource.data();
				const size_t size = wcslen(assetPath);
				ImGui::SetDragDropPayload("ASSETS_BROWSER_ITEM", assetPath, size * sizeof(wchar_t));
				ImGui::EndDragDropSource();
			}

			ImGui::PopStyleColor();

			if (ImGui::BeginPopupContextItem())
			{
				if (FileFormats::IsMeshIntermediate(format))
				{
					if (ImGui::MenuItem("Generate meshes"))
					{
						Serializer::LoadIntermediate(Utils::Erase(path.string(), m_RootDirectory.string() + "/"));
					}
				}
				if (format == FileFormats::Scene())
				{
					if (ImGui::MenuItem("Load Scene"))
					{ 
						auto callback = [weakScene = std::weak_ptr<Scene>(scene), path]()
						{
							if (std::shared_ptr<Scene> currentScene = weakScene.lock())
							{
								SceneManager::GetInstance().Delete(currentScene->GetName());
							}

							Serializer::DeserializeScene(path);
						};

						std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
						EventSystem::GetInstance().SendEvent(event);
					}
				}
				if (format == FileFormats::Mat())
				{
					if (ImGui::MenuItem("Clone"))
					{
						m_CloneMaterialMenu.opened = true;
						m_CloneMaterialMenu.material = MaterialManager::GetInstance().LoadMaterial(path);
						m_CloneMaterialMenu.name[0] = '\0';
					}
				}
				if (ImGui::MenuItem("Delete file"))
				{
					m_DeleteFileMenu.opened = true;
					m_DeleteFileMenu.filepath = path;
				}

				ImGui::EndPopup();
			}

			if (ImGui::IsItemHovered() && leftMouseButtonDoubleClicked)
			{
				if (directoryIter.is_directory())
				{
					m_CurrentDirectory /= directoryIter.path().filename();
				}
			}
			ImGui::TextWrapped("%s", filename.c_str());

			ImGui::NextColumn();
		}
		ImGui::Columns(1);

		ImGui::End();
	}
}

void Editor::SetDarkThemeColors()
{
	auto& colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

	// Headers
	colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Buttons
	colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Frame BG
	colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Tabs
	colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
	colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
	colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

	// Title
	colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
}

void Editor::Manipulate(const std::shared_ptr<Scene>& scene)
{
	if (!scene)
	{
		return;
	}

	if (scene->GetSelectedEntities().empty())
	{
		return;
	}

	for (const auto& [name, viewport] : ViewportManager::GetInstance().GetViewports())
	{
		if (!viewport || !viewport->GetCamera().lock())
		{
			continue;
		}

		auto callback = [this, viewport](
			const glm::vec2& position,
			const glm::ivec2 size,
			const std::shared_ptr<Entity>& camera,
			bool& active)
		{
			if (!viewport || !camera)
			{
				return;
			}

			if (viewport->IsHovered() && !Input::Mouse::IsMouseDown(Keycode::MOUSE_BUTTON_2))
			{
				if (Input::KeyBoard::IsKeyPressed(Keycode::KEY_W))
				{
					viewport->GetGizmoOperation() = ImGuizmo::OPERATION::TRANSLATE;
				}
				else if (Input::KeyBoard::IsKeyPressed(Keycode::KEY_R))
				{
					viewport->GetGizmoOperation() = ImGuizmo::OPERATION::ROTATE;
				}
				else if (Input::KeyBoard::IsKeyPressed(Keycode::KEY_S))
				{
					viewport->GetGizmoOperation() = ImGuizmo::OPERATION::SCALE;
				}
				else if (Input::KeyBoard::IsKeyPressed(Keycode::KEY_U))
				{
					viewport->GetGizmoOperation() = ImGuizmo::OPERATION::UNIVERSAL;
				}
				else if (Input::KeyBoard::IsKeyPressed(Keycode::KEY_Q))
				{
					viewport->GetGizmoOperation() = -1;
				}
			}

			if (viewport->GetGizmoOperation() == -1)
			{
				return;
			}

			if (camera->GetScene()->GetSelectedEntities().empty())
			{
				return;
			}

			const std::shared_ptr<Entity> entity = *camera->GetScene()->GetSelectedEntities().begin();
			if (!entity)
			{
				return;
			}

			ImGuizmo::SetOrthographic(true);
			ImGuizmo::SetDrawlist();

			ImGuizmo::SetRect(position.x, position.y, size.x, size.y);

			Camera& cameraComponent = camera->GetComponent<Camera>();
			glm::mat4 projectionMat4 = viewport->GetProjectionMat4();
			glm::mat4 viewMat4 = cameraComponent.GetViewMat4();
			Transform& transform = entity->GetComponent<Transform>();
			Transform& cameraTransform = camera->GetComponent<Transform>();

			if (glm::dot(cameraTransform.GetForward(), transform.GetPosition() - cameraTransform.GetPosition()) < 0)
			{
				return;
			}

			glm::mat4 transformMat4 = transform.GetTransform();
			ImGuizmo::Manipulate(glm::value_ptr(viewMat4), glm::value_ptr(projectionMat4),
				(ImGuizmo::OPERATION)viewport->GetGizmoOperation(), ImGuizmo::LOCAL, glm::value_ptr(transformMat4));

			active = ImGuizmo::IsUsing();
			if (active)
			{
				glm::vec3 position, rotation, scale;

				transformMat4 = glm::inverse(transform.GetTransform() *
					glm::inverse(transform.GetTransform(Transform::System::LOCAL))) *
					transformMat4;

				Utils::DecomposeTransform(transformMat4, position, rotation, scale);

				transform.Translate(position);
				transform.Rotate(rotation);
				transform.Scale(scale);
			}
		};

		viewport->SetDrawGizmosCallback(callback);
	}
}

void Editor::MoveCamera(const std::shared_ptr<Entity>& camera)
{
	if (!camera)
	{
		return;
	}

	Transform& transform = camera->GetComponent<Transform>();

	glm::vec3 rotation = transform.GetRotation();
	if (rotation.y > glm::two_pi<float>() || rotation.y < -glm::two_pi<float>())
	{
		rotation.y = 0.0f;
	}
	if (rotation.x > glm::two_pi<float>() || rotation.x < -glm::two_pi<float>())
	{
		rotation.x = 0.0f;
	}

	transform.Rotate(glm::vec3(rotation.x, rotation.y, 0.0f));

	constexpr double rotationSpeed = 90.0f;

	const glm::vec2 delta = Input::Mouse::GetMousePositionDelta() * rotationSpeed * Time::GetDeltaTime();

	transform.Rotate(glm::vec3(rotation.x - glm::radians(delta.y),
		rotation.y - glm::radians(delta.x), 0));

	constexpr float defaultSpeed = 2.0f;
	float speed = defaultSpeed;

	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_LEFT_SHIFT))
	{
		speed *= 10.0f;
	}

	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_W))
	{
		transform.Translate(transform.GetPosition() + transform.GetForward() * (float)Time::GetDeltaTime() * speed);
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::KEY_S))
	{
		transform.Translate(transform.GetPosition() + transform.GetForward() * -(float)Time::GetDeltaTime() * speed);
	}
	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_D))
	{
		transform.Translate(transform.GetPosition() + transform.GetRight() * (float)Time::GetDeltaTime() * speed);
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::KEY_A))
	{
		transform.Translate(transform.GetPosition() + transform.GetRight() * -(float)Time::GetDeltaTime() * speed);
	}

	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_Q))
	{
		transform.Translate(transform.GetPosition() + transform.GetUp() * -(float)Time::GetDeltaTime() * speed);
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::KEY_E))
	{
		transform.Translate(transform.GetPosition() + transform.GetUp() * (float)Time::GetDeltaTime() * speed);
	}
}

void Editor::ComponentsPopUpMenu(const std::shared_ptr<Entity>& entity)
{
	if (ImGui::BeginPopupContextWindow())
	{
		if (ImGui::MenuItem("Camera"))
		{
			if (entity->HasComponent<Transform>())
			{
				entity->AddComponent<Camera>(entity);
			}
		}
		if (ImGui::MenuItem("Renderer3D"))
		{
			entity->AddComponent<Renderer3D>();
		}
		else if (ImGui::MenuItem("PointLight"))
		{
			entity->AddComponent<PointLight>();
		}
		else if (ImGui::MenuItem("DirectionalLight"))
		{
			entity->AddComponent<DirectionalLight>();
		}
		ImGui::EndPopup();
	}
}

void Editor::MainMenuBar()
{
	ImGuiWindowClass windowClass;
	windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
	ImGui::SetNextWindowClass(&windowClass);

	ImGui::Begin("MainMenuBar", nullptr,
		ImGuiWindowFlags_MenuBar
		| ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoMove);

	ImGui::BeginMenuBar();
	if (ImGui::BeginMenu("Create"))
	{
		if (ImGui::MenuItem("Viewport"))
		{
			m_CreateViewportMenu.opened = true;
			m_CreateViewportMenu.name[0] = '\0';
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Tools"))
	{
		if (ImGui::MenuItem("Save Materials"))
		{
			MaterialManager::GetInstance().SaveAll();
		}
		if (ImGui::MenuItem("Reload Materials"))
		{
			MaterialManager::GetInstance().ReloadAll();
		}
		if (ImGui::MenuItem("Reload UUIDs"))
		{
			filepathByUuid.clear();
			uuidByFilepath.clear();
			Serializer::GenerateFilesUUID(std::filesystem::current_path());
		}
		ImGui::EndMenu();
	}
	ImGui::EndMenuBar();

	ImGui::End();
}

void Editor::Renderer3DComponent(const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<Renderer3D>())
	{
		return;
	}

	Renderer3D& r3d = entity->GetComponent<Renderer3D>();

	if (ImGui::Button("X"))
	{
		entity->RemoveComponent<Renderer3D>();
	}
	ImGui::SameLine();
	if (ImGui::CollapsingHeader("Renderer3D"))
	{
		Indent indent;

		if (r3d.mesh)
		{
			ImGui::Text("Mesh: %s", r3d.mesh->GetName().c_str());
		}
		else
		{
			ImGui::Text("Mesh: %s", none);
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSETS_BROWSER_ITEM"))
			{
				std::wstring filepath((const wchar_t*)payload->Data);
				filepath.resize(payload->DataSize / sizeof(wchar_t));

				if (FileFormats::Mesh() == Utils::GetFileFormat(filepath))
				{
					r3d.mesh = MeshManager::GetInstance().LoadMesh(Utils::GetShortFilepath(filepath));
				}
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::Text("Material:");
		ImGui::SameLine();
		if (r3d.material)
		{
			if (ImGui::Button(r3d.material->GetName().c_str()))
			{
				m_MaterialMenu.opened = true;
				m_MaterialMenu.material = r3d.material;
			}
		}
		else
		{
			ImGui::Button(none);
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSETS_BROWSER_ITEM"))
			{
				std::wstring filepath((const wchar_t*)payload->Data);
				filepath.resize(payload->DataSize / sizeof(wchar_t));

				if (Utils::GetFileFormat(filepath) == FileFormats::Mat())
				{
					r3d.material = MaterialManager::GetInstance().LoadMaterial(filepath);
				}
			}

			ImGui::EndDragDropTarget();
		}
	}
}

void Editor::PointLightComponent(const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<PointLight>())
	{
		return;
	}

	PointLight& pointLight = entity->GetComponent<PointLight>();

	if (ImGui::Button("X"))
	{
		entity->RemoveComponent<PointLight>();
	}
	ImGui::SameLine();
	if (ImGui::CollapsingHeader("PointLight"))
	{
		Indent indent;

		ImGui::ColorEdit3("Color", &pointLight.color[0]);
		ImGui::SliderFloat("Constant", &pointLight.constant, 0.0f, 1.0f);
		ImGui::SliderFloat("Linear", &pointLight.linear, 0.0f, 1.0f);
		ImGui::SliderFloat("Quadratic", &pointLight.quadratic, 0.0f, 1.0f);
	}
}

void Editor::DirectionalLightComponent(const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<DirectionalLight>())
	{
		return;
	}

	DirectionalLight& directionalLight = entity->GetComponent<DirectionalLight>();

	if (ImGui::Button("X"))
	{
		entity->RemoveComponent<DirectionalLight>();
	}
	ImGui::SameLine();
	if (ImGui::CollapsingHeader("DirectionalLight"))
	{
		Indent indent;

		ImGui::ColorEdit3("Color", &directionalLight.color[0]);
		ImGui::SliderFloat("Intensity", &directionalLight.intensity, 0.0f, 10.0f);
		ImGui::SliderFloat("Ambient", &directionalLight.ambient, 0.0f, 0.3f);
	}
}

Editor::Indent::Indent()
{
	ImGui::Indent();
}

Editor::Indent::~Indent()
{
	ImGui::Unindent();
}

void Editor::MaterialMenu::Update(Editor& editor)
{
	if (!material)
	{
		return;
	}

	if (opened && ImGui::Begin("Material", &opened))
	{
		if (ImGui::Button("Save"))
		{
			Material::Save(material);
		}

		if (ImGui::Button("Reload"))
		{
			Material::Reload(material);
		}

		ImGui::Text("Name: %s", material->GetName().c_str());
		ImGui::Text("Filepath: %s", material->GetFilepath().string().c_str());
		
		ImGui::Text("BaseMaterial: ");
		ImGui::SameLine();
		if (ImGui::Button(material->GetBaseMaterial()->GetFilepath().string().c_str()))
		{
			editor.m_BaseMaterialMenu.opened = true;
			editor.m_BaseMaterialMenu.baseMaterial = material->GetBaseMaterial();
		}

		if (ImGui::CollapsingHeader("Options"))
		{
			for (auto& [name, option] : material->GetOptionsByName())
			{
				if (ImGui::Checkbox(name.c_str(), &option.m_IsEnabled))
				{
					material->SetOption(name, option.m_IsEnabled);
				}
			}
		}

		for (const auto& [renderPassName, pipeline] : material->GetBaseMaterial()->GetPipelinesByRenderPass())
		{
			if (ImGui::CollapsingHeader(renderPassName.c_str()))
			{
				for (const auto& [set, uniformLayout] : pipeline->GetUniformLayouts())
				{
					const auto& descriptorSetIndex = pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::MATERIAL, renderPassName);
					if (!descriptorSetIndex || descriptorSetIndex.value() != set)
					{
						continue;
					}

					const std::shared_ptr<UniformWriter> uniformWriter = material->GetUniformWriter(renderPassName);

					for (const auto& binding : uniformLayout->GetBindings())
					{
						if (binding.type == ShaderReflection::Type::COMBINED_IMAGE_SAMPLER)
						{
							if (const std::shared_ptr<Texture>& texture = uniformWriter->GetTexture(binding.name))
							{
								ImGui::Text("%s", binding.name.c_str());
								ImGui::SameLine();
								ImGui::Text("%s", texture->GetFilepath().string().c_str());
								ImGui::Image(ImTextureID(texture->GetId()), { 128, 128 });

								if (ImGui::BeginDragDropTarget())
								{
									if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSETS_BROWSER_ITEM"))
									{
										std::wstring filepath((const wchar_t*)payload->Data);
										filepath.resize(payload->DataSize / sizeof(wchar_t));

										if (FileFormats::IsTexture(Utils::GetFileFormat(filepath)))
										{
											material->GetUniformWriter(renderPassName)->WriteTexture(binding.name, TextureManager::GetInstance().Load(filepath));
										}
									}

									ImGui::EndDragDropTarget();
								}
							}
						}
						else if (binding.type == ShaderReflection::Type::UNIFORM_BUFFER)
						{
							const std::shared_ptr<Buffer> buffer = material->GetBuffer(binding.name);
							if (!buffer)
							{
								continue;
							}

							ImGui::Text("%s", binding.name.c_str());

							void* data = (char*)buffer->GetData();

							std::function<void(const ShaderReflection::ReflectVariable&, bool&)> drawVariable = [&](
								const ShaderReflection::ReflectVariable& variable,
								bool& isChanged)
							{
								if (variable.type == ShaderReflection::ReflectVariable::Type::FLOAT)
								{
									isChanged += ImGui::SliderFloat(variable.name.c_str(), &Utils::GetValue<float>(data, variable.offset), 0.0f, 1.0f);
								}
								if (variable.type == ShaderReflection::ReflectVariable::Type::INT)
								{
									isChanged += ImGui::InputInt(variable.name.c_str(), &Utils::GetValue<int>(data, variable.offset));
								}
								if (variable.type == ShaderReflection::ReflectVariable::Type::VEC2)
								{
									isChanged += editor.DrawVec2Control(variable.name.c_str(), Utils::GetValue<glm::vec2>(data, variable.offset));
								}
								if (variable.type == ShaderReflection::ReflectVariable::Type::VEC3)
								{
									isChanged += editor.DrawVec3Control(variable.name.c_str(), Utils::GetValue<glm::vec3>(data, variable.offset));
								}
								if (variable.type == ShaderReflection::ReflectVariable::Type::VEC4)
								{
									isChanged += editor.DrawVec4Control(variable.name.c_str(), Utils::GetValue<glm::vec4>(data, variable.offset));
								}
								else if (variable.type == ShaderReflection::ReflectVariable::Type::STRUCT)
								{
									for (const auto& memberVariable : variable.variables)
									{
										drawVariable(memberVariable, isChanged);
									}
								}
							};

							bool isChanged = false;
							for (const auto& variable : binding.buffer->variables)
							{
								drawVariable(variable, isChanged);
							}

							if (isChanged)
							{
								// Need to mark for flush that the buffer was changed.
								buffer->WriteToBuffer(data, 0, 0);
							}
						}
					}
				}
			}
		}

		ImGui::End();
	}
}

void Editor::CreateFileMenu::Update()
{
	if (!opened)
	{
		return;
	}

	ImGui::SetNextWindowSize({ 360.0f, 60.0f });
	ImGui::SetNextWindowPos(ImGui::GetWindowViewport()->GetCenter(), 0, { 0.5f, 0.0f });
	if (ImGui::Begin("Create File", &opened,
		ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::InputText("Filename", name, 64);
		
		ImGui::SameLine();

		if (ImGui::Button("Create"))
		{
			if (name[0] != '\0')
			{
				MakeFile(filepath / (name + format));

				opened = false;
			}
		}

		ImGui::End();
	}
}

void Editor::CreateFileMenu::MakeFile(const std::filesystem::path& filepath)
{
	std::ofstream out(filepath, std::ostream::binary);
	out.close();
}

void Editor::DeleteFileMenu::Update()
{
	if (!opened)
	{
		return;
	}

	ImGui::SetNextWindowSize({ 250.0f, 60.0f });
	ImGui::SetNextWindowPos(ImGui::GetWindowViewport()->GetCenter(), 0, { 0.5f, 0.0f });
	if (ImGui::Begin("Deleting directory or file", &opened,
		ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::Text("Are you sure you want to delete\n%s?", filepath.filename().c_str());
		if (ImGui::Button("Yes"))
		{
			if (Utils::GetFileFormat(filepath.string()).empty())
			{
				std::filesystem::remove_all(filepath);
			}
			else
			{
				std::filesystem::remove(filepath);
			}

			opened = false;
		}

		ImGui::SameLine();

		if (ImGui::Button("No"))
		{
			opened = false;
		}
		ImGui::End();
	}
}

void Editor::CreateViewportMenu::Update(const Editor& editor)
{
	if (!opened)
	{
		return;
	}

	ImGui::SetNextWindowSize({ 350.0f, 80.0f });
	ImGui::SetNextWindowPos(ImGui::GetWindowViewport()->GetCenter(), 0, { 0.5f, 0.0f });
	if (ImGui::Begin("Create Viewport", &opened,
		ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::InputText("Name", name, 64);
		
		ImGui::SameLine();

		if (ImGui::Button("Create"))
		{
			if (name[0] != '\0')
			{
				auto callback = [this]()
				{
					if (const std::shared_ptr<Viewport> viewport = ViewportManager::GetInstance().GetViewport(name))
					{
						return;
					}

					ViewportManager::GetInstance().Create(name, size);
					opened = false;
					size = { 1024, 1024 };
					name[0] = '\0';
				};

				std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
				EventSystem::GetInstance().SendEvent(event);
			}
		}

		editor.DrawIVec2Control("Size", size, 1024, { 0, 2560 });

		ImGui::End();
	}
}

void Editor::BaseMaterialMenu::Update(const Editor& editor)
{
	if (!baseMaterial)
	{
		return;
	}

	if (opened && ImGui::Begin("BaseMaterial", &opened))
	{
		if (ImGui::Button("Reload"))
		{
			BaseMaterial::Reload(baseMaterial);
		}

		ImGui::Text("Name: %s", baseMaterial->GetName().c_str());
		ImGui::Text("Filepath: %s", baseMaterial->GetFilepath().string().c_str());

		ImGui::End();
	}
}

void Editor::CloneMaterialMenu::Update()
{
	if (!opened)
	{
		return;
	}

	ImGui::SetNextWindowSize({ 450.0f, 70.0f });
	ImGui::SetNextWindowPos(ImGui::GetWindowViewport()->GetCenter(), 0, { 0.5f, 0.0f });
	if (ImGui::Begin("Clone Material", &opened,
		ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::Text(std::string("Cloned Material: " + material->GetFilepath().string()).c_str());
		ImGui::InputText("Filename", name, 64);

		ImGui::SameLine();

		if (ImGui::Button("Clone"))
		{
			if (name[0] != '\0')
			{
				std::filesystem::path filepath = material->GetFilepath().parent_path() / name;
				filepath.replace_extension(FileFormats::Mat());
				std::shared_ptr<Material> clonedMaterial = MaterialManager::GetInstance().Clone(name, filepath, material);
				Material::Save(clonedMaterial);

				opened = false;
			}
		}

		ImGui::End();
	}
}
