#include "Editor.h"

#include "../Core/Input.h"
#include "../Core/FileFormatNames.h"
#include "../Core/MaterialManager.h"
#include "../Core/MeshManager.h"
#include "../Core/TextureManager.h"
#include "../Core/Serializer.h"
#include "../Core/Time.h"
#include "../Core/WindowManager.h"
#include "../Core/ViewportManager.h"
#include "../EventSystem/EventSystem.h"
#include "../EventSystem/NextFrameEvent.h"
#include "../EventSystem/ResizeEvent.h"
#include "../Editor/ImGuizmo.h"

using namespace Pengine;

Editor::Editor()
{
	SetDarkThemeColors();
}

void Editor::Update(std::shared_ptr<Scene> scene)
{
	Manipulate();

	Hierarchy(scene);
	Properties(scene);
	AssetBrowser();

	m_MaterialMenu.Update(*this);

	ImGui::Begin("Settings");
	ImGui::Text("FPS: %.0f", 1.0f / (float)Time::GetDeltaTime());
	ImGui::Text("DrawCalls: %d", drawCallsCount);
	ImGui::Text("Triangles: %d", (int)vertexCount);
	drawCallsCount = 0;
	vertexCount = 0;
	ImGui::Text("Meshes: %d", (int)MeshManager::GetInstance().GetMeshes().size());
	ImGui::Text("BaseMaterials: %d", (int)MaterialManager::GetInstance().GetBaseMaterials().size());
	ImGui::Text("Materials: %d", (int)MaterialManager::GetInstance().GetMaterials().size());
	ImGui::Text("Textures: %d", (int)TextureManager::GetInstance().GetTextures().size());
	ImGui::End();

	if (Input::Mouse::IsMouseReleased(Keycode::MOUSE_BUTTON_2))
	{
		m_MovingCamera = nullptr;
	}

	for (const auto& [name, viewport] : ViewportManager::GetInstance().GetViewports())
	{
		if (viewport->IsHovered() && Input::Mouse::IsMouseDown(Keycode::MOUSE_BUTTON_2))
		{
			m_MovingCamera = viewport->GetCamera();
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
}

bool Editor::DrawVec2Control(const std::string& label, glm::vec2& values, float resetValue, const glm::vec2& limits, float speed, float columnWidth)
{
	bool changed = false;

	ImGuiIO& io = ImGui::GetIO();

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

bool Editor::DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue,
	const glm::vec2& limits, float speed, float columnWidth)
{
	bool changed = false;

	ImGuiIO& io = ImGui::GetIO();

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

bool Editor::DrawVec4Control(const std::string& label, glm::vec4& values, float resetValue, const glm::vec2& limits, float speed, float columnWidth)
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

bool Editor::DrawIVec2Control(const std::string& label, glm::ivec2& values, float resetValue, const glm::vec2& limits, float speed, float columnWidth)
{
	bool changed = false;

	ImGuiIO& io = ImGui::GetIO();

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
		values.y = resetValue;
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

bool Editor::DrawIVec3Control(const std::string& label, glm::ivec3& values, float resetValue, const glm::vec2& limits, float speed, float columnWidth)
{
	bool changed = false;

	ImGuiIO& io = ImGui::GetIO();

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
		values.y = resetValue;
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
		values.z = resetValue;
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

bool Editor::DrawIVec4Control(const std::string& label, glm::ivec4& values, float resetValue, const glm::vec2& limits, float speed, float columnWidth)
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
		values.x = resetValue;
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
		values.y = resetValue;
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
		values.z = resetValue;
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
		values.w = resetValue;
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

void Editor::Hierarchy(std::shared_ptr<Scene> scene)
{
	if (ImGui::Begin("Hierarchy"))
	{
		if (ImGui::CollapsingHeader("Game Objects"))
		{
			DrawScene(scene);
		}

		GameObjectPopUpMenu(scene);

		ImGui::End();
	}
}

void Editor::DrawScene(std::shared_ptr<Scene> scene)
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
				auto callback = [scene, uuid]()
				{
					std::shared_ptr<Entity> entity = scene->FindEntityByUUID(uuid);
					if (entity)
					{
						if (std::shared_ptr<Entity> parent = entity->GetParent())
						{
							parent->RemoveChild(entity);
						}
					}
				};

				NextFrameEvent* event = new NextFrameEvent(callback, Event::Type::OnNextFrame, this);
				EventSystem::GetInstance().SendEvent(event);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::TreePop();

		const std::vector<std::shared_ptr<Entity>>& entities = scene->GetEntities();
		for (std::shared_ptr<Entity> entity : entities)
		{
			if (entity->HasParent())
			{
				continue;
			}

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
			flags |= m_SelectedEntities.count(entity->GetUUID()) ? 
				ImGuiTreeNodeFlags_Selected : 0;
			
			DrawNode(entity, flags);
		}
	}
}

void Editor::DrawNode(std::shared_ptr<Entity> entity, ImGuiTreeNodeFlags flags)
{
	flags |= entity->GetChilds().size() == 0 ? ImGuiTreeNodeFlags_Leaf : 0;

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
			auto callback = [entity, uuid]()
			{
				std::shared_ptr<Entity> child = entity->GetScene()->FindEntityByUUID(uuid);
				if (child)
				{
					if (entity->HasAsParent(child, true) || (entity->HasParent() && entity->GetParent() == child))
					{
						return;
					}

					if (std::shared_ptr<Entity> parent = child->GetParent())
					{
						parent->RemoveChild(child);
					}

					entity->AddChild(child);
				}
			};

			NextFrameEvent* event = new NextFrameEvent(callback, Event::Type::OnNextFrame, this);
			EventSystem::GetInstance().SendEvent(event);
		}
		ImGui::EndDragDropTarget();
	}

	if (ImGui::IsItemHovered() && Input::Mouse::IsMouseReleased(Keycode::MOUSE_BUTTON_1))
	{
		if (!Input::KeyBoard::IsKeyDown(Keycode::KEY_LEFT_CONTROL))
		{
			m_SelectedEntities.clear();
		}
		
		m_SelectedEntities.emplace(entity->GetUUID());
	}

	if (opened)
	{
		ImGui::TreePop();
		DrawChilds(entity);
	}
}

void Editor::DrawChilds(std::shared_ptr<Entity> entity)
{
	for (std::shared_ptr<Entity> child : entity->GetChilds())
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= m_SelectedEntities.count(child->GetUUID()) ?
			ImGuiTreeNodeFlags_Selected : 0;

		DrawNode(child, flags);
	}
}


void Editor::Properties(std::shared_ptr<Scene> scene)
{
	if (ImGui::Begin("Properties"))
	{
		for (const std::string entityUUID : m_SelectedEntities)
		{
			std::shared_ptr<Entity> entity = scene->FindEntityByUUID(entityUUID);
			if (!entity)
			{
				continue;
			}

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
			strcpy_s(name, entity->GetName().c_str());
			if (ImGui::InputText("Name", name, sizeof(name)))
			{
				entity->SetName(name);
			}

			ComponentsPopUpMenu(entity);

			TransformComponent(entity);
			CameraComponent(entity);
			Renderer3DComponent(entity);
			PointLightComponent(entity);

			ImGui::NewLine();
		}
		ImGui::End();
	}
}

void Editor::CameraComponent(std::shared_ptr<Entity> entity)
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

		int type = (int)camera.GetType();
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

		std::shared_ptr<Viewport> viewport = ViewportManager::GetInstance().GetViewport("Main");
		if (viewport->GetCamera() == entity)
		{
			bool setToMainViewport = true;
			ImGui::Checkbox("Set to main viewport", &setToMainViewport);
		}
		else
		{
			bool setToMainViewport = false;
			if (ImGui::Checkbox("Set to main viewport", &setToMainViewport))
			{
				viewport->SetCamera(entity);
			}
		}
	}
}

void Editor::TransformComponent(std::shared_ptr<Entity> entity)
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

void Editor::GameObjectPopUpMenu(std::shared_ptr<Scene> scene)
{
	if (ImGui::BeginPopupContextWindow())
	{
		if (ImGui::MenuItem("Create Gameobject"))
		{
			std::shared_ptr<Entity> entity = scene->CreateEntity();
			entity->AddComponent<Transform>(entity);
		}

		if (!m_SelectedEntities.empty())
		{
			if (ImGui::MenuItem("Delete Selected"))
			{
				for (const std::string& uuid : m_SelectedEntities)
				{
					auto callback = [scene, uuid]()
					{
						if (std::shared_ptr<Entity> entity = scene->FindEntityByUUID(uuid))
						{
							scene->DeleteEntity(entity);
						}
					};

					NextFrameEvent* event = new NextFrameEvent(callback, Event::Type::OnNextFrame, this);
					EventSystem::GetInstance().SendEvent(event);
				}
				m_SelectedEntities.clear();
			}
		}

		if (ImGui::MenuItem("Save Scene"))
		{
			std::string sceneFilepath = scene->GetFilepath();
			if (sceneFilepath == none)
			{
				sceneFilepath = "Scenes/" + scene->GetName() + FileFormats::Scene();
			}
			Serializer::SerializeScene(sceneFilepath, scene);
		}
		
		ImGui::EndPopup();
	}
}

void Editor::AssetBrowser()
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

		const ImTextureID folderIconId = (ImTextureID)TextureManager::GetInstance().GetTexture("Editor/Images/FolderIcon.png")->GetId();
		const ImTextureID fileIconId = (ImTextureID)TextureManager::GetInstance().GetTexture("Editor/Images/FileIcon.png")->GetId();
		const ImTextureID metaIconId = (ImTextureID)TextureManager::GetInstance().GetTexture("Editor/Images/MetaIcon.png")->GetId();
		const ImTextureID materialIconId = (ImTextureID)TextureManager::GetInstance().GetTexture("Editor/Images/MaterialIcon.png")->GetId();
		const ImTextureID meshIconId = (ImTextureID)TextureManager::GetInstance().GetTexture("Editor/Images/MeshIcon.png")->GetId();

		bool iconHovered = false;

		for (auto& directoryIter : std::filesystem::directory_iterator(m_CurrentDirectory))
		{
			const std::string path = Utils::Replace(directoryIter.path().string(), '\\', '/');
			if (Utils::Contains(path, ".cpp") || Utils::Contains(path, ".h"))
			{
				continue;
			}

			const std::string filename = Utils::EraseDirectoryFromFilePath(path);
			const std::string format = Utils::GetFileFormat(path);
			ImTextureID currentIcon;

			if (FileFormats::IsAsset(format))
			{
				if (!std::filesystem::exists(path + FileFormats::Meta()))
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
			else if (FileFormats::IsTexture(path))
			{
				if (std::shared_ptr<Texture> texture = TextureManager::GetInstance().GetTexture(path))
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
			ImGui::ImageButton(currentIcon, { thumbnailSize, thumbnailSize }, ImVec2(0, 1), ImVec2(1, 0));
			ImGui::PopID();

			iconHovered = iconHovered ? iconHovered : ImGui::IsItemHovered();

			if (ImGui::BeginDragDropSource())
			{
				const char* assetPath = path.c_str();
				size_t size = strlen(assetPath);
				ImGui::SetDragDropPayload("ASSETS_BROWSER_ITEM", assetPath, size);
				ImGui::EndDragDropSource();
			}

			ImGui::PopStyleColor();

			if (ImGui::BeginPopupContextItem())
			{
				if (FileFormats::IsMeshIntermediate(format))
				{
					if (ImGui::MenuItem("Generate meshes"))
					{
						Serializer::LoadIntermediate(Utils::Erase(path, m_RootDirectory.string() + "/"));
					}
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

void Editor::Manipulate()
{
	if (!Input::Mouse::IsMouseDown(Keycode::MOUSE_BUTTON_2))
	{
		if (Input::KeyBoard::IsKeyPressed(Keycode::KEY_W))
		{
			m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
		}
		else if (Input::KeyBoard::IsKeyPressed(Keycode::KEY_R))
		{
			m_GizmoOperation = ImGuizmo::OPERATION::ROTATE;
		}
		else if (Input::KeyBoard::IsKeyPressed(Keycode::KEY_S))
		{
			m_GizmoOperation = ImGuizmo::OPERATION::SCALE;
		}
		else if (Input::KeyBoard::IsKeyPressed(Keycode::KEY_U))
		{
			m_GizmoOperation = ImGuizmo::OPERATION::UNIVERSAL;
		}
		else if (Input::KeyBoard::IsKeyPressed(Keycode::KEY_Q))
		{
			m_GizmoOperation = -1;
		}
	}

	if (m_SelectedEntities.empty() || m_GizmoOperation == -1)
	{
		return;
	}

	for (const auto& [name, viewport] : ViewportManager::GetInstance().GetViewports())
	{
		if (!viewport || !viewport->GetCamera())
		{
			continue;
		}

		auto callback = [this](const glm::vec2& position, const glm::ivec2 size, std::shared_ptr<Entity> camera)
		{
			if (m_SelectedEntities.empty())
			{
				return;
			}

			std::shared_ptr<Entity> entity = camera->GetScene()->FindEntityByUUID(*m_SelectedEntities.begin());
			if (!entity)
			{
				return;
			}

			ImGuizmo::SetOrthographic(true);
			ImGuizmo::SetDrawlist();

			ImGuizmo::SetRect(position.x, position.y, size.x, size.y);

			Camera& cameraComponent = camera->GetComponent<Camera>();
			glm::mat4 projectionMat4 = cameraComponent.GetProjectionMat4();
			glm::mat4 viewMat4 = cameraComponent.GetViewMat4();
			Transform& transform = entity->GetComponent<Transform>();
			glm::mat4 transformMat4 = transform.GetTransform();
			ImGuizmo::Manipulate(glm::value_ptr(viewMat4), glm::value_ptr(projectionMat4),
				(ImGuizmo::OPERATION)m_GizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(transformMat4));

			if (ImGuizmo::IsUsing())
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

void Editor::MoveCamera(std::shared_ptr<Entity> camera)
{
	if (!camera)
	{
		return;
	}

	Camera& cameraComponent = camera->GetComponent<Camera>();
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

	const float rotationSpeed = 1.0f;

	glm::vec2 delta = Input::Mouse::GetMousePositionDelta() * 0.1;

	transform.Rotate(glm::vec3(rotation.x - glm::radians(delta.y),
		rotation.y - glm::radians(delta.x), 0));

	const float defaultSpeed = 2.0f;
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

	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_LEFT_CONTROL))
	{
		transform.Translate(transform.GetPosition() + transform.GetUp() * -(float)Time::GetDeltaTime() * speed);
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::SPACE))
	{
		transform.Translate(transform.GetPosition() + transform.GetUp() * (float)Time::GetDeltaTime() * speed);
	}
}

void Editor::ComponentsPopUpMenu(std::shared_ptr<Entity> entity)
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
		ImGui::EndPopup();
	}
}

void Editor::Renderer3DComponent(std::shared_ptr<Entity> entity)
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
				std::string path((const char*)payload->Data);
				path.resize(payload->DataSize);

				if (FileFormats::Mesh() == Utils::GetFileFormat(path))
				{
					path = Utils::Erase(path, m_RootDirectory.string() + "/");

					r3d.mesh = MeshManager::GetInstance().LoadMesh(path);
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
				std::string path((const char*)payload->Data);
				path.resize(payload->DataSize);

				if (Utils::GetFileFormat(path) == FileFormats::Mat())
				{
					path = Utils::Erase(path, m_RootDirectory.string() + "/");

					r3d.material = MaterialManager::GetInstance().LoadMaterial(path);
				}
			}

			ImGui::EndDragDropTarget();
		}
	}
}

void Editor::PointLightComponent(std::shared_ptr<Entity> entity)
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

	if (ImGui::Begin("Material", &opened))
	{
		if (ImGui::Button("Save"))
		{
			Material::Save(material);
		}

		ImGui::Text("Name: %s", material->GetName().c_str());
		ImGui::Text("Filepath: %s", material->GetFilepath().c_str());

		for (auto [renderPass, pipeline] : material->GetBaseMaterial()->GetPipelinesByRenderPass())
		{
			if (ImGui::CollapsingHeader(renderPass.c_str()))
			{
				for (auto [location, binding] : pipeline->GetChildUniformLayout()->GetBindingsByLocation())
				{
					if (binding.type == UniformLayout::Type::SAMPLER)
					{
						if (std::shared_ptr<Texture> texture = material->GetTexture(binding.name))
						{
							ImGui::Text("%s", texture->GetFilepath().c_str());
							ImGui::Image(ImTextureID(texture->GetId()), { 128, 128 });

							if (ImGui::BeginDragDropTarget())
							{
								if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSETS_BROWSER_ITEM"))
								{
									std::string path((const char*)payload->Data);
									path.resize(payload->DataSize);

									if (FileFormats::IsTexture(Utils::GetFileFormat(path)))
									{
										material->SetTexture(binding.name, TextureManager::GetInstance().Load(path));
									}
								}

								ImGui::EndDragDropTarget();
							}
						}
					}
					else if (binding.type == UniformLayout::Type::BUFFER)
					{
						ImGui::Text("%s", binding.name.c_str());

						auto buffer = pipeline->GetBuffer(binding.name);
						void* data = (char*)buffer->GetData() + buffer->GetInstanceSize() * material->GetIndex();
						for (const auto& variable : binding.values)
						{
							if (!variable.name.empty() && variable.name[0] == '_')
							{
								continue;
							}

							if (variable.type == "vec2")
							{
								editor.DrawVec2Control(variable.name, Utils::GetValue<glm::vec2>(data, variable.offset));
							}
							else if (variable.type == "vec3")
							{
								editor.DrawVec3Control(variable.name, Utils::GetValue<glm::vec3>(data, variable.offset));
							}
							else if (variable.type == "vec4")
							{
								editor.DrawVec4Control(variable.name, Utils::GetValue<glm::vec4>(data, variable.offset));
							}
							else if (variable.type == "color")
							{
								ImGui::ColorEdit4(variable.name.c_str(), &Utils::GetValue<glm::vec4>(data, variable.offset)[0]);
							}
							else if (variable.type == "float")
							{
								ImGui::InputFloat(variable.name.c_str(), &Utils::GetValue<float>(data, variable.offset));
							}
							else if (variable.type == "int")
							{
								ImGui::InputInt(variable.name.c_str(), &Utils::GetValue<int>(data, variable.offset));
							}
							else if (variable.type == "sampler")
							{
								ImGui::InputInt(variable.name.c_str(), &Utils::GetValue<int>(data, variable.offset));
							}
						}
					}
				}
			}
		}

		ImGui::End();
	}
}
