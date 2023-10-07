#include "Editor.h"

#include "../Core/Input.h"
#include "../Core/FileFormatNames.h"
#include "../Core/MaterialManager.h"
#include "../Core/MeshManager.h"
#include "../Core/TextureManager.h"
#include "../Core/Serializer.h"
#include "../Core/Time.h"

using namespace Pengine;

Editor::Editor()
{
}

void Editor::Update(std::shared_ptr<Scene> scene)
{
	Hierarchy(scene);
	Properties(scene);
	AssetBrowser();

	m_MaterialMenu.Update(*this);

	ImGui::Begin("Settings");
	ImGui::Text("FPS: %.0f", 1.0f / Time::GetDeltaTime());
	ImGui::Text("DrawCalls: %d", drawCallsCount);
	ImGui::Text("Triangles: %d", vertexCount);
	drawCallsCount = 0;
	vertexCount = 0;
	ImGui::Text("Meshes: %d", (int)MeshManager::GetInstance().GetMeshes().size());
	ImGui::Text("BaseMaterials: %d", (int)MaterialManager::GetInstance().GetBaseMaterials().size());
	ImGui::Text("Materials: %d", (int)MaterialManager::GetInstance().GetMaterials().size());
	ImGui::Text("Textures: %d", (int)TextureManager::GetInstance().GetTextures().size());
	ImGui::End();
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
		GameObjectPopUpMenu(scene);

		if (ImGui::CollapsingHeader("Game Objects"))
		{
			DrawScene(scene);
		}
		ImGui::End();
	}
}

void Editor::DrawScene(std::shared_ptr<Scene> scene)
{
	Indent indent;

	if (ImGui::TreeNodeEx((void*)&scene, 0, scene->GetName().c_str()))
	{
		ImGui::TreePop();

		const std::vector<GameObject*>& gameObjects = scene->GetGameObjects();
		for (auto& gameObject : gameObjects)
		{
			if (gameObject->GetOwner())
			{
				continue;
			}

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
			flags |= m_SelectedGameObjects.count(gameObject->GetUUID()) ? 
				ImGuiTreeNodeFlags_Selected : 0;
			
			DrawNode(gameObject, flags);
		}
	}
}

void Editor::DrawNode(GameObject* gameObject, ImGuiTreeNodeFlags flags)
{
	if (!gameObject->IsEditorVisible())
	{
		return;
	}
	flags |= gameObject->GetChilds().size() == 0 ? ImGuiTreeNodeFlags_Leaf : 0;

	Indent indent;

	ImGui::PushID(gameObject);
	bool enabled = gameObject->IsEnabled();
	if (ImGui::Checkbox("##IsEnabled", &enabled))
	{
		gameObject->SetEnabled(enabled);
	}
	ImGui::PopID();

	ImGui::SameLine();

	ImGuiStyle* style = &ImGui::GetStyle();

	const bool opened = ImGui::TreeNodeEx((void*)gameObject, flags, gameObject->GetName().c_str());
	style->Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

	if (ImGui::IsItemHovered() && Input::Mouse::IsMouseReleased(Keycode::MOUSE_BUTTON_1))
	{
		if (!Input::KeyBoard::IsKeyDown(Keycode::KEY_LEFT_CONTROL))
		{
			m_SelectedGameObjects.clear();
		}
		
		m_SelectedGameObjects.emplace(gameObject->GetUUID());
	}

	if (opened)
	{
		ImGui::TreePop();
		DrawChilds(gameObject);
	}
}

void Editor::DrawChilds(GameObject* gameObject)
{
	for (size_t childIndex = 0; childIndex < gameObject->GetChilds().size(); childIndex++)
	{
		GameObject* child = gameObject->GetChilds()[childIndex];
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= m_SelectedGameObjects.count(gameObject->GetUUID()) ?
			ImGuiTreeNodeFlags_Selected : 0;

		DrawNode(child, flags);
	}
}


void Editor::Properties(std::shared_ptr<Scene> scene)
{
	if (ImGui::Begin("Properties"))
	{
		for (const std::string gameObjectUUID : m_SelectedGameObjects)
		{
			GameObject* gameObject = scene->FindGameObjectByUUID(gameObjectUUID);
			if (!gameObject)
			{
				continue;
			}

			ComponentsPopUpMenu(gameObject);

			ImGui::Text("UUID: %s", gameObject->GetUUID().c_str());

			char name[64];
			strcpy_s(name, gameObject->GetName().c_str());
			if (ImGui::InputText("Name", name, sizeof(name)))
			{
				gameObject->SetName(name);
			}

			ImGui::Text("Owner: %s", gameObject->HasOwner() ? gameObject->GetOwner()->GetName().c_str() : "Null");
			//ImGui::Checkbox("Is Serializable", &gameObject->m_IsSerializable);
			//ImGui::Checkbox("Is Selectable", &gameObject->m_IsSelectable);

			TransformComponent(gameObject->m_Transform);
			Renderer3DComponent(gameObject->m_ComponentManager.GetComponent<Renderer3D>());
			PointLightComponent(gameObject->m_ComponentManager.GetComponent<PointLight>());

			ImGui::NewLine();
		}
		ImGui::End();
	}
}

void Editor::TransformComponent(Transform& transform)
{
	if (ImGui::Button("O"))
	{
	}
	ImGui::SameLine();
	if (ImGui::CollapsingHeader("Transform"))
	{
		Indent indent;

		ImGui::Checkbox("Follow owner", &transform.m_FollowOwner);
		glm::vec3 position = transform.GetPosition(Transform::System::LOCAL);
		glm::vec3 rotation = glm::degrees(transform.GetRotation(Transform::System::LOCAL));
		glm::vec3 scale = transform.GetScale(Transform::System::LOCAL);
		DrawVec3Control("Translation", position);
		DrawVec3Control("Rotation", rotation, 0.0f, { -360.0f, 360.0f }, 1.0f);
		DrawVec3Control("Scale", scale, 1.0f, { 0.0f, 25.0f });
		transform.Scale(scale);
		transform.Translate(position);
		transform.Rotate(glm::radians(rotation));
	}
}

void Editor::GameObjectPopUpMenu(std::shared_ptr<Scene> scene)
{
	if (ImGui::BeginPopupContextWindow())
	{
		if (ImGui::MenuItem("Create gameobject"))
		{
			scene->CreateGameObject();
		}

		if (!m_SelectedGameObjects.empty())
		{
			if (ImGui::MenuItem("Delete selected"))
			{
				for (const std::string& gameObjectUUID : m_SelectedGameObjects)
				{
					if (GameObject* gameObject = scene->FindGameObjectByUUID(gameObjectUUID))
					{
						scene->DeleteGameObject(gameObject);
					}
				}
				m_SelectedGameObjects.clear();
			}
			else if (ImGui::MenuItem("Duplicate selected"))
			{
				auto selected = m_SelectedGameObjects;
				for (const std::string& gameObjectUUID : selected)
				{
					if (GameObject* gameObject = scene->FindGameObjectByUUID(gameObjectUUID))
					{
						GameObject* newGameObject = scene->CreateGameObject();
						newGameObject->Copy(*gameObject);
						m_SelectedGameObjects.emplace(newGameObject->GetUUID());
					}
				}
			}
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

			const std::string filename = Utils::RemoveDirectoryFromFilePath(path);
			const std::string format = Utils::GetFileFormat(path);
			ImTextureID currentIcon;

			if (FileFormats::IsAsset(format))
			{
				if (!std::filesystem::exists(path + "." + FileFormats::Meta()))
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

void Editor::ComponentsPopUpMenu(GameObject* gameObject)
{
	if (ImGui::BeginPopupContextWindow())
	{
		if (ImGui::MenuItem("Renderer3D"))
		{
			gameObject->m_ComponentManager.AddComponent<Renderer3D>();
		}
		else if (ImGui::MenuItem("PointLight"))
		{
			gameObject->m_ComponentManager.AddComponent<PointLight>();
		}
		ImGui::EndPopup();
	}
}

void Editor::Renderer3DComponent(Renderer3D* r3d)
{
	if (!r3d)
	{
		return;
	}

	if (ImGui::Button("x"))
	{
	}
	ImGui::SameLine();
	if (ImGui::CollapsingHeader("Renderer3D"))
	{
		Indent indent;

		if (r3d->mesh)
		{
			ImGui::Text("Mesh: %s", r3d->mesh->GetName().c_str());
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

					r3d->mesh = MeshManager::GetInstance().LoadMesh(path);
				}
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::Text("Material:");
		ImGui::SameLine();
		if (r3d->material)
		{
			if (ImGui::Button(r3d->material->GetName().c_str()))
			{
				m_MaterialMenu.opened = true;
				m_MaterialMenu.material = r3d->material;
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

					r3d->material = MaterialManager::GetInstance().LoadMaterial(path);
				}
			}

			ImGui::EndDragDropTarget();
		}
	}
}

void Editor::PointLightComponent(PointLight* pointLight)
{
	if (!pointLight)
	{
		return;
	}

	if (ImGui::Button("x"))
	{
	}
	ImGui::SameLine();
	if (ImGui::CollapsingHeader("PointLight"))
	{
		Indent indent;

		ImGui::ColorEdit3("Color", &pointLight->color[0]);
		ImGui::SliderFloat("Constant", &pointLight->constant, 0.0f, 1.0f);
		ImGui::SliderFloat("Linear", &pointLight->linear, 0.0f, 1.0f);
		ImGui::SliderFloat("Quadratic", &pointLight->quadratic, 0.0f, 1.0f);
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
