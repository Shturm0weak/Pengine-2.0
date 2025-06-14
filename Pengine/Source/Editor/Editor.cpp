#include "Editor.h"

#include "../Components/Camera.h"
#include "../Components/DirectionalLight.h"
#include "../Components/PointLight.h"
#include "../Components/Renderer3D.h"
#include "../Components/SkeletalAnimator.h"
#include "../Components/Transform.h"
#include "../Components/Canvas.h"
#include "../Core/AsyncAssetLoader.h"
#include "../Core/FileFormatNames.h"
#include "../Core/Input.h"
#include "../Core/KeyCode.h"
#include "../Core/SceneManager.h"
#include "../Core/MaterialManager.h"
#include "../Core/MeshManager.h"
#include "../Core/Serializer.h"
#include "../Core/TextureManager.h"
#include "../Core/ThreadPool.h"
#include "../Core/Time.h"
#include "../Core/ViewportManager.h"
#include "../Core/Viewport.h"
#include "../Core/WindowManager.h"
#include "../Core/RenderPassManager.h"
#include "../Core/RenderPassOrder.h"
#include "../Core/ClayScriptManager.h"
#include "../Core/Profiler.h"
#include "../Editor/ImGuizmo.h"
#include "../EventSystem/EventSystem.h"
#include "../EventSystem/NextFrameEvent.h"

#include "../Graphics/Device.h"
#include "../Graphics/Renderer.h"

#include <fstream>
#include <format>

#include <assimp/postprocess.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace Pengine;

Editor::Editor()
{
	SetDarkThemeColors();
	//ViewportManager::GetInstance().Create("Main", { 800, 800 });

	m_AssetBrowserFilterBuffer[0] = '\0';

	m_Thumbnails.Initialize();
}

void Editor::Update(const std::shared_ptr<Scene>& scene, Window& window)
{
	PROFILER_SCOPE(__FUNCTION__);

	Input& input = Input::GetInstance(&window);

	if (input.IsMouseReleased(Keycode::MOUSE_BUTTON_2))
	{
		m_MovingCamera = nullptr;
	}

	for (const auto& [name, viewport] : window.GetViewportManager().GetViewports())
	{
		if (viewport->IsHovered() && input.IsMouseDown(Keycode::MOUSE_BUTTON_2))
		{
			m_MovingCamera = viewport->GetCamera().lock();
		}

		if (m_MovingCamera)
		{
			WindowManager::GetInstance().GetCurrentWindow()->DisableCursor();
			MoveCamera(m_MovingCamera, window);
			break;
		}
		else
		{
			WindowManager::GetInstance().GetCurrentWindow()->ShowCursor();
		}
	}

	if (input.IsKeyDown(Keycode::KEY_LEFT_CONTROL) && input.IsKeyPressed(Keycode::KEY_F))
	{
		m_FullScreen = !m_FullScreen;
	}

	if (m_FullScreen)
	{
		return;
	}

	Manipulate(scene, window);

	MainMenuBar(scene);
	Hierarchy(scene, window);
	SceneInfo(scene);
	Properties(scene, window);
	AssetBrowser(scene);
	AssetBrowserHierarchy();

	m_MaterialMenu.Update(*this);
	m_BaseMaterialMenu.Update(*this);
	m_CreateFileMenu.Update();
	m_DeleteFileMenu.Update();
	m_CloneMaterialMenu.Update();
	m_CreateViewportMenu.Update(*this, window);
	m_LoadIntermediateMenu.Update();
	m_TextureMetaPropertiesMenu.Update();
	m_ImportMenu.Update(*this);

	m_Thumbnails.UpdateThumbnails();

	ImGui::Begin("Settings");
	ImGui::Text("FPS: %.0f", 1.0f / static_cast<float>(Time::GetDeltaTime()));
	ImGui::Text("DrawCalls: %d", drawCallsCount);
	ImGui::Text("Triangles: %d", static_cast<int>(vertexCount));
	ImGui::Text("Meshes: %d", static_cast<int>(MeshManager::GetInstance().GetMeshes().size()));
	ImGui::Text("BaseMaterials: %d", static_cast<int>(MaterialManager::GetInstance().GetBaseMaterials().size()));
	ImGui::Text("Materials: %d", static_cast<int>(MaterialManager::GetInstance().GetMaterials().size()));
	ImGui::Text("Textures: %d", static_cast<int>(TextureManager::GetInstance().GetTextures().size()));
	ImGui::Text("VRAM Allocated: %d", static_cast<int>(vramAllocated / 1024));

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

bool Editor::ImageCheckBox(const void* id, ImTextureID textureOn, ImTextureID textureOff, bool& enabled)
{
	const ImTextureID isEnabledIcon = enabled ? textureOn : textureOff;

	bool clicked = false;

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushID(id);
	if (ImGui::ImageButton(isEnabledIcon, { ImGui::GetFontSize(), ImGui::GetFontSize() }))
	{
		clicked = true;
		enabled = !enabled;
	}
	ImGui::PopID();
	ImGui::PopStyleColor();

	return clicked;
}

void Editor::Hierarchy(const std::shared_ptr<Scene>& scene, Window& window)
{
	if (ImGui::Begin("Scene Hierarchy"))
	{
		Input& input = Input::GetInstance(&window);

		if (ImGui::IsWindowFocused() && input.IsKeyDown(Keycode::KEY_LEFT_CONTROL) && input.IsKeyPressed(Keycode::KEY_A))
		{
			scene->GetSelectedEntities().clear();
			for (const std::shared_ptr<Entity> entity : scene->GetEntities())
			{
				scene->GetSelectedEntities().emplace(entity);
			}
		}

		if (scene)
		{
			DrawScene(scene, window);

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
			// Generate thumbnail for the scene if there is no one.
			m_Thumbnails.GetOrGenerateThumbnail(m_RootDirectory / scene->GetFilepath(), scene, Thumbnails::Type::SCENE);

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

void Editor::DrawScene(const std::shared_ptr<Scene>& scene, Window& window)
{
	Indent indent;

	if (ImGui::TreeNodeEx((void*)&scene, 0, scene->GetName().c_str()))
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT"))
			{
				UUID* uuidPtr = (UUID*)payload->Data;
				auto callback = [weakScene = std::weak_ptr<Scene>(scene), uuid = *uuidPtr]()
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
			
			DrawNode(entity, flags, window);
		}
	}
}

void Editor::DrawNode(const std::shared_ptr<Entity>& entity, ImGuiTreeNodeFlags flags, Window& window)
{
	flags |= entity->GetChilds().empty() ? ImGuiTreeNodeFlags_Leaf : 0;

	Indent indent;

	bool enabled = entity->IsEnabled();

	const ImTextureID showIconId = (ImTextureID)TextureManager::GetInstance().GetTexture(std::filesystem::path("Editor") / "Images" / "ShowIcon.png")->GetId();
	const ImTextureID hideIconId = (ImTextureID)TextureManager::GetInstance().GetTexture(std::filesystem::path("Editor") / "Images" / "HideIcon.png")->GetId();

	if (ImageCheckBox(entity.get(), showIconId, hideIconId, enabled))
	{
		entity->SetEnabled(enabled);
	}

	ImGui::SameLine();

	ImGuiStyle* style = &ImGui::GetStyle();
	if (entity->IsPrefab())
	{
		style->Colors[ImGuiCol_Text] = ImVec4(0.557f, 0.792f, 0.902f, 1.0f);
	}

	const bool opened = ImGui::TreeNodeEx((void*)entity.get(), flags, entity->GetName().c_str());
	style->Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

	if (ImGui::BeginDragDropSource())
	{
		ImGui::SetDragDropPayload("GAMEOBJECT", (const void*)&entity->GetUUID(), sizeof(UUID));
		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT"))
		{
			UUID* uuidPtr = (UUID*)payload->Data;
			auto callback = [weakEntity = std::weak_ptr<Entity>(entity), uuid = *uuidPtr]()
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

	if (ImGui::IsItemClicked() && (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) > ImGui::GetTreeNodeToLabelSpacing())
	{
		auto& selectedEntities = entity->GetScene()->GetSelectedEntities();
		if (!Input::GetInstance(&window).IsKeyDown(Keycode::KEY_LEFT_CONTROL))
		{
			selectedEntities.clear();
		}
		
		if (selectedEntities.contains(entity))
		{
			selectedEntities.erase(entity);
		}
		else
		{
			selectedEntities.emplace(entity);
		}
	}

	if (opened)
	{
		ImGui::TreePop();
		DrawChilds(entity, window);
	}
}

void Editor::DrawChilds(const std::shared_ptr<Entity>& entity, Window& window)
{
	for (const std::weak_ptr<Entity> weakChild : entity->GetChilds())
	{
		if (const std::shared_ptr<Entity> child = weakChild.lock())
		{
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
			flags |= entity->GetScene()->GetSelectedEntities().count(child) ?
				ImGuiTreeNodeFlags_Selected : 0;

			DrawNode(child, flags, window);
		}
	}
}

void Editor::Properties(const std::shared_ptr<Scene>& scene, Window& window)
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

			ImGui::Text("UUID: %zu%zu", entity->GetUUID().GetUpper(), entity->GetUUID().GetLower());

			char name[256];
			strcpy(name, entity->GetName().c_str());
			if (ImGui::InputText("Name", name, sizeof(name)))
			{
				entity->SetName(name);
			}

			if (ImGui::CollapsingHeader("Prefab"))
			{
				bool validUUID = false;
				if (entity->IsPrefab())
				{
					ImGui::PushID("Deattach Prefab");
					if (ImGui::Button("X"))
					{
						entity->SetPrefabFilepathUUID(UUID(0, 0));
					}
					ImGui::PopID();

					ImGui::SameLine();
					
					const std::filesystem::path prefabFilepath = Utils::FindFilepath(entity->GetPrefabFilepathUUID());
					if (std::filesystem::exists(prefabFilepath))
					{
						ImGui::Text("Filepath: %s", prefabFilepath.string().c_str());
					
						validUUID = true;

						ImGui::PushID("Save Prefab");
						if (ImGui::Button("Save"))
						{
							Serializer::SerializePrefab(prefabFilepath, entity);
							
							auto callback = [scene, entity]()
							{
								std::vector<std::shared_ptr<Entity>> entitiesToDelete;
								for (const auto& maybePrefabEntity : scene->GetEntities())
								{
									if (maybePrefabEntity != entity && maybePrefabEntity->GetPrefabFilepathUUID() == entity->GetPrefabFilepathUUID())
									{
										entitiesToDelete.emplace_back(maybePrefabEntity);
									}
								}

								for (auto& entityToDelete : entitiesToDelete)
								{
									std::shared_ptr<Entity> clonedEntity = scene->CloneEntity(entity);

									if (clonedEntity->HasParent())
									{
										clonedEntity->GetParent()->RemoveChild(clonedEntity);
									}

									if (entityToDelete->HasParent())
									{
										entityToDelete->GetParent()->AddChild(clonedEntity);
									}

									clonedEntity->GetComponent<Transform>().Copy(entityToDelete->GetComponent<Transform>());
								}

								for (auto& entityToDelete : entitiesToDelete)
								{
									scene->DeleteEntity(entityToDelete);
								}
							};

							std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
							EventSystem::GetInstance().SendEvent(event);
						}
						ImGui::PopID();
					}
					else
					{
						ImGuiStyle* style = &ImGui::GetStyle();
						auto previousColor = style->Colors[ImGuiCol_Text];
						style->Colors[ImGuiCol_Text] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
						ImGui::Text("Filepath: prefab uuid is invalid!");
						style->Colors[ImGuiCol_Text] = previousColor;
					}
				}
				else
				{
					ImGui::PushID("Save Prefab");
					if (ImGui::Button("Save as prefab"))
					{
						const std::filesystem::path prefabFilepath = Utils::GetShortFilepath(m_CurrentDirectory / (entity->GetName() + FileFormats::Prefab()));
						entity->SetPrefabFilepathUUID(Serializer::GenerateFileUUID(prefabFilepath));
						SaveScene(entity->GetScene());
					}
					ImGui::PopID();
				}
			}

			ComponentsPopUpMenu(entity);

			TransformComponent(entity);
			CameraComponent(entity, window);
			Renderer3DComponent(entity);
			PointLightComponent(entity);
			DirectionalLightComponent(entity);
			SkeletalAnimatorComponent(entity);
			CanvasComponent(entity);

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
			ImGui::PushID("SSAO Quality");
			isChangedToSerialize += ImGui::Combo("Quality", &graphicsSettings.ssao.resolutionScale, resolutionScales, 4);
			ImGui::PopID();

			ImGui::PushID("SSAO Blur Quality");
			isChangedToSerialize += ImGui::Combo("Blur Quality", &graphicsSettings.ssao.resolutionBlurScale, resolutionScales, 4);
			ImGui::PopID();

			ImGui::PushID("SSAO Bias");
			isChangedToSerialize += ImGui::SliderFloat("Bias", &graphicsSettings.ssao.bias, 0.0f, 10.0f);
			ImGui::PopID();
			
			ImGui::PushID("SSAO Radius");
			isChangedToSerialize += ImGui::SliderFloat("Radius", &graphicsSettings.ssao.radius, 0.0f, 1.0f);
			ImGui::PopID();
			
			ImGui::PushID("SSAO Kernel Size");
			isChangedToSerialize += ImGui::SliderInt("Kernel Size", &graphicsSettings.ssao.kernelSize, 2, 64);
			ImGui::PopID();
			
			ImGui::PushID("SSAO Noise Size");
			isChangedToSerialize += ImGui::SliderInt("Noise Size", &graphicsSettings.ssao.noiseSize, 4, 64);
			ImGui::PopID();
			
			ImGui::PushID("SSAO AO Scale");
			isChangedToSerialize += ImGui::SliderFloat("AO Scale", &graphicsSettings.ssao.aoScale, 0.0f, 10.0f);
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Shadows"))
		{
			ImGui::PushID("Shadows Is Enabled");
			isChangedToSerialize += ImGui::Checkbox("Is Enabled", &graphicsSettings.shadows.isEnabled);
			ImGui::PopID();

			const char* const qualities[] = { "1024", "2048", "4096" };
			ImGui::PushID("Shadows Quality");
			isChangedToSerialize += ImGui::Combo("Quality", &graphicsSettings.shadows.quality, qualities, 3);
			ImGui::PopID();

			ImGui::PushID("Shadows Cascade Count");
			if (ImGui::SliderInt("Cascade Count", &graphicsSettings.shadows.cascadeCount, 2, 10))
			{
				isChangedToSerialize += true;

				graphicsSettings.shadows.biases.resize(graphicsSettings.shadows.cascadeCount, 0.0f);
			}
			ImGui::PopID();

			ImGui::PushID("Shadows Visualize");
			ImGui::Checkbox("Visualize", &graphicsSettings.shadows.visualize);
			ImGui::PopID();

			ImGui::PushID("Shadows Pcf Enabled");
			isChangedToSerialize += ImGui::Checkbox("Pcf Enabled", &graphicsSettings.shadows.pcfEnabled);
			ImGui::PopID();

			ImGui::PushID("Shadows Pcf Range");
			isChangedToSerialize += ImGui::SliderInt("Pcf Range", &graphicsSettings.shadows.pcfRange, 1, 5);
			ImGui::PopID();

			ImGui::PushID("Shadows Split Factor");
			isChangedToSerialize += ImGui::SliderFloat("Split Factor", &graphicsSettings.shadows.splitFactor, 0.0f, 1.0f);
			ImGui::PopID();

			ImGui::PushID("Shadows Max Distance");
			isChangedToSerialize += ImGui::SliderFloat("Max Distance", &graphicsSettings.shadows.maxDistance, 0.0f, 1000.0f);
			ImGui::PopID();

			ImGui::PushID("Shadows Fog Factor");
			isChangedToSerialize += ImGui::SliderFloat("Fog Factor", &graphicsSettings.shadows.fogFactor, 0.0f, 1.0f);
			ImGui::PopID();

			for (size_t i = 0; i < graphicsSettings.shadows.biases.size(); i++)
			{
				const std::string biasName = "Bias " + std::to_string(i);
				const std::string idName = "Shadows " + biasName;
				ImGui::PushID(idName.c_str());
				isChangedToSerialize += ImGui::SliderFloat(biasName.c_str(), &graphicsSettings.shadows.biases[i], 0.0f, 1.0f);
				ImGui::PopID();
			}
		}

		if (ImGui::CollapsingHeader("Bloom"))
		{
			ImGui::PushID("Bloom Is Enabled");
			isChangedToSerialize += ImGui::Checkbox("Is Enabled", &graphicsSettings.bloom.isEnabled);
			ImGui::PopID();

			const char* const resolutionScales[] = { "0.25", "0.5", "0.75", "1.0" };
			ImGui::PushID("Bloom Quality");
			isChangedToSerialize += ImGui::Combo("Quality", &graphicsSettings.bloom.resolutionScale, resolutionScales, 4);
			ImGui::PopID();

			ImGui::PushID("Bloom Mip Count");
			isChangedToSerialize += ImGui::SliderInt("Mip Count", &graphicsSettings.bloom.mipCount, 1, 10);
			ImGui::PopID();

			ImGui::PushID("Bloom Intensity");
			isChangedToSerialize += ImGui::SliderFloat("Intensity", &graphicsSettings.bloom.intensity, 0.0f, 2.0f);
			ImGui::PopID();

			ImGui::PushID("Bloom Brightness Threshold");
			isChangedToSerialize += ImGui::SliderFloat("Brightness Threshold", &graphicsSettings.bloom.brightnessThreshold, 0.0f, 2.0f);
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("SSR"))
		{
			ImGui::PushID("SSR Is Enabled");
			isChangedToSerialize += ImGui::Checkbox("Is Enabled", &graphicsSettings.ssr.isEnabled);
			ImGui::PopID();

			const char* const resolutionScales[] = { "0.25", "0.5", "0.75", "1.0" };
			ImGui::PushID("SSR Quality");
			isChangedToSerialize += ImGui::Combo("Quality", &graphicsSettings.ssr.resolutionScale, resolutionScales, 4);
			ImGui::PopID();

			const char* const resolutionBlurScales[] = { "0.125", "0.25", "0.5", "0.75", "1.0" };
			ImGui::PushID("SSR Blur Quality");
			isChangedToSerialize += ImGui::Combo("Blur Quality", &graphicsSettings.ssr.resolutionBlurScale, resolutionBlurScales, 5);
			ImGui::PopID();

			ImGui::PushID("SSR Max Distance");
			isChangedToSerialize += ImGui::SliderFloat("Max Distance", &graphicsSettings.ssr.maxDistance, 1.0f, 100.0f);
			ImGui::PopID();

			ImGui::PushID("SSR Resolution");
			isChangedToSerialize += ImGui::SliderFloat("Resolution", &graphicsSettings.ssr.resolution, 0.0f, 1.0f);
			ImGui::PopID();

			ImGui::PushID("SSR Step Count");
			isChangedToSerialize += ImGui::SliderInt("Step Count", &graphicsSettings.ssr.stepCount, 0, 20);
			ImGui::PopID();

			ImGui::PushID("SSR Thickness");
			isChangedToSerialize += ImGui::SliderFloat("Thickness", &graphicsSettings.ssr.thickness, 0.0f, 5.0f);
			ImGui::PopID();

			ImGui::PushID("SSR Blur Range");
			isChangedToSerialize += ImGui::SliderInt("Blur Range", &graphicsSettings.ssr.blurRange, 0, 10);
			ImGui::PopID();

			ImGui::PushID("SSR Blur Offset");
			isChangedToSerialize += ImGui::SliderInt("Blur Offset", &graphicsSettings.ssr.blurOffset, 0, 10);
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Post Process"))
		{
			ImGui::PushID("Post Process FXAA");
			isChangedToSerialize += ImGui::Checkbox("FXAA", &graphicsSettings.postProcess.fxaa);
			ImGui::PopID();

			ImGui::PushID("Post Process Gamma");
			isChangedToSerialize += ImGui::SliderFloat("Gamma", &graphicsSettings.postProcess.gamma, 0.0f, 3.0f);
			ImGui::PopID();

			const char* const toneMappers[] = {"NONE", "ACES" };
			int* toneMapperIndex = (int*)(&graphicsSettings.postProcess.toneMapper);
			ImGui::PushID("Post Process Tone Mapper");
			isChangedToSerialize += ImGui::Combo("Tone Mapper", toneMapperIndex, toneMappers, (int)GraphicsSettings::PostProcess::ToneMapper::COUNT);
			ImGui::PopID();
		}

		if (isChangedToSerialize && std::filesystem::exists(graphicsSettings.GetFilepath()))
		{
			Serializer::SerializeGraphicsSettings(graphicsSettings);
		}
	}
}

void Editor::CameraComponent(const std::shared_ptr<Entity>& entity, Window& window)
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
			for (const auto& viewport : window.GetViewportManager().GetViewports())
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
			for (const auto& passName : passPerViewportOrder)
			{
				if (ImGui::MenuItem(passName.c_str()))
				{
					camera.SetPassName(passName);
					camera.SetRenderTargetIndex(0);
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Render Target Index"))
		{
			if (!camera.GetPassName().empty())
			{
				if (RenderPassManager::GetInstance().GetPass(camera.GetPassName())->GetType() == Pass::Type::GRAPHICS)
				{
					const int indexCount = RenderPassManager::GetInstance().GetRenderPass(camera.GetPassName())->GetAttachmentDescriptions().size();
					for (size_t i = 0; i < indexCount; i++)
					{
						if (ImGui::MenuItem(std::to_string(i).c_str()))
						{
							camera.SetRenderTargetIndex(i);
						}
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

	ImGui::PushID("Transform O");
	if (ImGui::Button("O"))
	{
	}
	ImGui::PopID();
	
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
			const std::shared_ptr<Entity> entity = scene->CreateEntity();
			entity->AddComponent<Transform>(entity);

			scene->GetSelectedEntities().clear();
			scene->GetSelectedEntities().emplace(entity);
		}

		if (ImGui::MenuItem("Clone Gameobject"))
		{
			std::set<std::shared_ptr<Entity>> clonedEntities;
			for (std::shared_ptr<Entity> entity : scene->GetSelectedEntities())
			{
				clonedEntities.emplace(scene->CloneEntity(entity));
			}

			scene->GetSelectedEntities().clear();
			scene->GetSelectedEntities() = std::move(clonedEntities);
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
			SaveScene(scene);
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
		}
		else
		{
			ImGui::Button("<-");
		}

		ImGui::SameLine();

		ImGui::PushItemWidth(32 * ImGui::GetFontSize());
		ImGui::InputTextWithHint("##AssetBrowserFilter", "Search", m_AssetBrowserFilterBuffer, 64);
		ImGui::PopItemWidth();

		{
			std::vector<std::filesystem::path> paths;
			std::filesystem::path currentPath = m_CurrentDirectory;
			paths.emplace_back(currentPath);

			while (currentPath != m_RootDirectory)
			{
				currentPath = currentPath.parent_path();
				paths.emplace_back(currentPath);
			}

			for (auto path = paths.rbegin(); path != paths.rend(); path++)
			{
				ImGui::SameLine();

				std::string pathString = path->string();
				const size_t index = pathString.find_last_of('\\');
				pathString = pathString.substr(index + 1, pathString.size() - index);

				if (ImGui::Button(pathString.c_str()))
				{
					m_CurrentDirectory = *path;
				}

				ImGui::SameLine();

				ImGui::Text("/");
			}
		}

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
			const std::string filename = path.filename().string();
			const std::string format = Utils::GetFileFormat(path);

			if (format == ".cpp"
				|| format == ".h"
				|| format == FileFormats::Meta())
			{
				continue;
			}
			
			if (FileFormats::IsAsset(format))
			{
				std::filesystem::path metaFilePath = path;
				metaFilePath.concat(FileFormats::Meta());
				if (!std::filesystem::exists(metaFilePath))
				{
					Serializer::GenerateFileUUID(path);
				}
			}

			if (m_AssetBrowserFilterBuffer[0] != '\0' && !Utils::Contains(Utils::ToLower(filename), m_AssetBrowserFilterBuffer))
			{
				continue;
			}

			ThumbnailAtlas::TileInfo fileIconInfo = GetFileIcon(directoryIter.path(), format);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			ImGui::PushID(filename.c_str());
			if (ImGui::ImageButton(
				fileIconInfo.atlasId,
				{ thumbnailSize, thumbnailSize },
				ImVec2(fileIconInfo.uvStart.x, fileIconInfo.uvStart.y),
				ImVec2(fileIconInfo.uvEnd.x, fileIconInfo.uvEnd.y)))
			{

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
						m_ImportMenu = {};
						m_ImportMenu.opened = true;
						m_ImportMenu.filepath = path;
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
				else if (format == FileFormats::BaseMat())
				{
					if (ImGui::MenuItem("Load"))
					{
						m_BaseMaterialMenu.opened = true;
						m_BaseMaterialMenu.baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(path);
					}
				}
				if (format == FileFormats::Mat())
				{
					if (ImGui::MenuItem("Load"))
					{
						m_MaterialMenu.opened = true;
						m_MaterialMenu.material = MaterialManager::GetInstance().LoadMaterial(path);
					}
					if (ImGui::MenuItem("Clone"))
					{
						m_CloneMaterialMenu.opened = true;
						m_CloneMaterialMenu.material = MaterialManager::GetInstance().LoadMaterial(path);
						m_CloneMaterialMenu.name[0] = '\0';
					}
				}
				if (FileFormats::IsTexture(format))
				{
					if (ImGui::MenuItem("Meta"))
					{
						m_TextureMetaPropertiesMenu.opened = true;
						m_TextureMetaPropertiesMenu.meta = *Serializer::DeserializeTextureMeta(path.string() + FileFormats::Meta());
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

void Editor::AssetBrowserHierarchy()
{
	if (ImGui::Begin("Asset Browser Hierarchy"))
	{
		DrawAssetBrowserHierarchy(m_RootDirectory);

		ImGui::End();
	}
}

void Editor::DrawAssetBrowserHierarchy(const std::filesystem::path& directory)
{
	Indent indent;

	for (auto& directoryIter : std::filesystem::directory_iterator(directory))
	{
		const std::filesystem::path path = Utils::GetShortFilepath(directoryIter.path());
		if (Utils::Contains(path.string(), ".cpp") || Utils::Contains(path.string(), ".h"))
		{
			continue;
		}

		const bool isDirectory = std::filesystem::is_directory(path);
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
		if (!isDirectory)
		{
			flags |= ImGuiTreeNodeFlags_Leaf;
		}

		ThumbnailAtlas::TileInfo fileIconInfo = GetFileIcon(path, Utils::GetFileFormat(path));
		ImGui::Image(
			fileIconInfo.atlasId,
			ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()),
			ImVec2(fileIconInfo.uvStart.x, fileIconInfo.uvStart.y),
			ImVec2(fileIconInfo.uvEnd.x, fileIconInfo.uvEnd.y));
		ImGui::SameLine();

		const std::string filename = path.filename().string();
		const bool opened = ImGui::TreeNodeEx(filename.c_str(), flags);

		if (isDirectory)
		{
			if (ImGui::IsItemClicked() && (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) > ImGui::GetTreeNodeToLabelSpacing())
			{
				m_CurrentDirectory = directoryIter.path();
			}
		}

		if (opened)
		{
			ImGui::TreePop();

			if (isDirectory)
			{
				DrawAssetBrowserHierarchy(directoryIter.path());
			}
		}
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

void Editor::Manipulate(const std::shared_ptr<Scene>& scene, Window& window)
{
	if (!scene)
	{
		return;
	}

	if (scene->GetSelectedEntities().empty())
	{
		return;
	}

	for (const auto& [name, viewport] : window.GetViewportManager().GetViewports())
	{
		if (!viewport || !viewport->GetCamera().lock())
		{
			continue;
		}

		auto callback = [this, &window, viewport](
			const glm::vec2& position,
			const glm::ivec2 size,
			const std::shared_ptr<Entity>& camera,
			bool& active)
		{
			if (!viewport || !camera)
			{
				return;
			}

			Input& input = Input::GetInstance(&window);

			if (viewport->IsHovered() && !input.IsMouseDown(Keycode::MOUSE_BUTTON_2))
			{
				if (input.IsKeyPressed(Keycode::KEY_W))
				{
					viewport->GetGizmoOperation() = ImGuizmo::OPERATION::TRANSLATE;
				}
				else if (input.IsKeyPressed(Keycode::KEY_R))
				{
					viewport->GetGizmoOperation() = ImGuizmo::OPERATION::ROTATE;
				}
				else if (input.IsKeyPressed(Keycode::KEY_S))
				{
					viewport->GetGizmoOperation() = ImGuizmo::OPERATION::SCALE;
				}
				else if (input.IsKeyPressed(Keycode::KEY_U))
				{
					viewport->GetGizmoOperation() = ImGuizmo::OPERATION::UNIVERSAL;
				}
				else if (input.IsKeyPressed(Keycode::KEY_Q))
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

void Editor::MoveCamera(const std::shared_ptr<Entity>& camera, Window& window)
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

	Input& input = Input::GetInstance(&window);

	const glm::vec2 delta = input.GetMousePositionDelta() * rotationSpeed * Time::GetDeltaTime();

	transform.Rotate(glm::vec3(rotation.x - glm::radians(delta.y),
		rotation.y - glm::radians(delta.x), 0));

	constexpr float defaultSpeed = 2.0f;
	float speed = defaultSpeed;

	if (input.IsKeyDown(Keycode::KEY_LEFT_SHIFT))
	{
		speed *= 10.0f;
	}

	if (input.IsKeyDown(Keycode::KEY_W))
	{
		transform.Translate(transform.GetPosition() + transform.GetForward() * (float)Time::GetDeltaTime() * speed);
	}
	else if (input.IsKeyDown(Keycode::KEY_S))
	{
		transform.Translate(transform.GetPosition() + transform.GetForward() * -(float)Time::GetDeltaTime() * speed);
	}
	if (input.IsKeyDown(Keycode::KEY_D))
	{
		transform.Translate(transform.GetPosition() + transform.GetRight() * (float)Time::GetDeltaTime() * speed);
	}
	else if (input.IsKeyDown(Keycode::KEY_A))
	{
		transform.Translate(transform.GetPosition() + transform.GetRight() * -(float)Time::GetDeltaTime() * speed);
	}

	if (input.IsKeyDown(Keycode::KEY_Q))
	{
		transform.Translate(transform.GetPosition() + transform.GetUp() * -(float)Time::GetDeltaTime() * speed);
	}
	else if (input.IsKeyDown(Keycode::KEY_E))
	{
		transform.Translate(transform.GetPosition() + transform.GetUp() * (float)Time::GetDeltaTime() * speed);
	}
}

ThumbnailAtlas::TileInfo Editor::GetFileIcon(const std::filesystem::path& filepath, const std::string& format)
{
	ThumbnailAtlas::TileInfo finalFileIconInfo{};
	finalFileIconInfo.uvStart = { 0.0f, 1.0f };
	finalFileIconInfo.uvEnd = { 1.0f, 0.0f };

	const std::filesystem::path editorImagesPath = std::filesystem::path("Editor") / "Images";

	auto getThumbnail = [&](Thumbnails::Type type, const std::filesystem::path& defaultFilepath)
	{
		ThumbnailAtlas::TileInfo fileIconInfo = m_Thumbnails.GetOrGenerateThumbnail(filepath, nullptr, type);
		if (fileIconInfo.atlasId)
		{
			finalFileIconInfo = fileIconInfo;
		}
		else
		{
			finalFileIconInfo.atlasId = (ImTextureID)TextureManager::GetInstance().Load(defaultFilepath)->GetId();
		}
	};

	if (std::filesystem::is_directory(filepath))
	{
		finalFileIconInfo.atlasId = (ImTextureID)TextureManager::GetInstance().Load(editorImagesPath / "FolderIcon.png")->GetId();
	}
	else if (format == FileFormats::Meta())
	{
		finalFileIconInfo.atlasId = (ImTextureID)TextureManager::GetInstance().Load(editorImagesPath / "MetaIcon.png")->GetId();
	}
	else if (format == FileFormats::Mat())
	{
		getThumbnail(Thumbnails::Type::MAT, editorImagesPath / "MaterialIcon.png");
	}
	else if (format == FileFormats::Mesh())
	{
		getThumbnail(Thumbnails::Type::MESH, editorImagesPath / "MeshIcon.png");
	}
	else if (format == FileFormats::Scene())
	{
		ThumbnailAtlas::TileInfo fileIconInfo = m_Thumbnails.TryGetThumbnail(filepath);
		if (fileIconInfo.atlasId)
		{
			finalFileIconInfo = fileIconInfo;
		}
		else
		{
			finalFileIconInfo.atlasId = (ImTextureID)TextureManager::GetInstance().Load(editorImagesPath / "FileIcon.png")->GetId();
		}
	}
	else if (format == FileFormats::Prefab())
	{
		getThumbnail(Thumbnails::Type::PREFAB, editorImagesPath / "FileIcon.png");
	}
	else if (FileFormats::IsTexture(format))
	{
		getThumbnail(Thumbnails::Type::TEXTURE, editorImagesPath / "FileIcon.png");
	}

	if (!finalFileIconInfo.atlasId)
	{
		finalFileIconInfo.atlasId = (ImTextureID)TextureManager::GetInstance().Load(editorImagesPath / "FileIcon.png")->GetId();
	}
	return finalFileIconInfo;
}

void Editor::SaveScene(std::shared_ptr<Scene> scene)
{
	std::string sceneFilepath = scene->GetFilepath().string();
	if (sceneFilepath == none)
	{
		sceneFilepath = "Scenes/" + scene->GetName() + FileFormats::Scene();
	}
	Serializer::SerializeScene(sceneFilepath, scene);
	m_Thumbnails.GetOrGenerateThumbnail(m_RootDirectory / sceneFilepath, scene, Thumbnails::Type::SCENE);
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
		else if (ImGui::MenuItem("SkeletalAnimator"))
		{
			entity->AddComponent<SkeletalAnimator>();
		}
		else if (ImGui::MenuItem("Canvas"))
		{
			entity->AddComponent<Canvas>();
		}
		ImGui::EndPopup();
	}
}

void Editor::MainMenuBar(const std::shared_ptr<Scene>& scene)
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
	if (ImGui::BeginMenu("Create"))
	{
		if (ImGui::MenuItem("Create Gameobject"))
		{
			const std::shared_ptr<Entity> entity = scene->CreateEntity();
			entity->AddComponent<Transform>(entity);
		}
		if (ImGui::MenuItem("Cube"))
		{
			scene->CreateCube();
		}
		if (ImGui::MenuItem("Camera"))
		{
			scene->CreateCamera();
		}
		if (ImGui::MenuItem("Directional Light"))
		{
			scene->CreateDirectionalLight();
		}
		if (ImGui::MenuItem("Point Light"))
		{
			scene->CreatePointLight();
		}
		if (ImGui::MenuItem("Canvas"))
		{
			scene->CreateCanvas();
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

	ImGui::PushID("Renderer3D X");
	if (ImGui::Button("X"))
	{
		entity->RemoveComponent<Renderer3D>();
	}
	ImGui::PopID();
	
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

		ImGui::PushID("R3D Is Enabled");
		ImGui::Checkbox("Is Enabled", &r3d.isEnabled);
		ImGui::PopID();

		const char* const renderingOrder[] = { "-5", "-4", "-3", "-2", "-1", "0", "1", "2", "3", "4", "5", };
		ImGui::PushID("R3D Rendering Order");
		ImGui::Combo("Rendering Order", &r3d.renderingOrder, renderingOrder, 11);
		ImGui::PopID();
	}
}

void Editor::PointLightComponent(const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<PointLight>())
	{
		return;
	}

	PointLight& pointLight = entity->GetComponent<PointLight>();

	ImGui::PushID("PointLight X");
	if (ImGui::Button("X"))
	{
		entity->RemoveComponent<PointLight>();
	}
	ImGui::PopID();

	ImGui::SameLine();

	if (ImGui::CollapsingHeader("PointLight"))
	{
		Indent indent;

		ImGui::ColorEdit3("Color", &pointLight.color[0]);
		ImGui::SliderFloat("Intensity", &pointLight.intensity, 0.0f, 10.0f);
		ImGui::SliderFloat("Radius", &pointLight.radius, 0.0f, 10.0f);
		ImGui::Checkbox("Draw Bounding Sphere", &pointLight.drawBoundingSphere);
	}
}

void Editor::DirectionalLightComponent(const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<DirectionalLight>())
	{
		return;
	}

	DirectionalLight& directionalLight = entity->GetComponent<DirectionalLight>();

	ImGui::PushID("DirectionalLight X");
	if (ImGui::Button("X"))
	{
		entity->RemoveComponent<DirectionalLight>();
	}
	ImGui::PopID();
	
	ImGui::SameLine();
	
	if (ImGui::CollapsingHeader("DirectionalLight"))
	{
		Indent indent;

		ImGui::ColorEdit3("Color", &directionalLight.color[0]);
		ImGui::SliderFloat("Intensity", &directionalLight.intensity, 0.0f, 10.0f);
		ImGui::SliderFloat("Ambient", &directionalLight.ambient, 0.0f, 0.3f);
	}
}

void Editor::SkeletalAnimatorComponent(const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<SkeletalAnimator>())
	{
		return;
	}

	SkeletalAnimator& skeletalAnimator = entity->GetComponent<SkeletalAnimator>();

	ImGui::PushID("SkeletalAnimator X");
	if (ImGui::Button("X"))
	{
		entity->RemoveComponent<SkeletalAnimator>();
	}
	ImGui::PopID();

	ImGui::SameLine();
	
	if (ImGui::CollapsingHeader("SkeletalAnimator"))
	{
		Indent indent;

		if (skeletalAnimator.GetSkeleton())
		{
			ImGui::Text("Skeleton: %s", skeletalAnimator.GetSkeleton()->GetName().c_str());
		}
		else
		{
			ImGui::Text("Skeleton: %s", none);
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSETS_BROWSER_ITEM"))
			{
				std::wstring filepath((const wchar_t*)payload->Data);
				filepath.resize(payload->DataSize / sizeof(wchar_t));

				if (Utils::GetFileFormat(filepath) == FileFormats::Skeleton())
				{
					skeletalAnimator.SetSkeleton(MeshManager::GetInstance().LoadSkeleton(Utils::GetShortFilepath(filepath)));
				}
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::Text("Animation:");
		ImGui::SameLine();
		if (skeletalAnimator.GetSkeletalAnimation())
		{
			ImGui::Button(skeletalAnimator.GetSkeletalAnimation()->GetName().c_str());
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

				if (Utils::GetFileFormat(filepath) == FileFormats::Anim())
				{
					skeletalAnimator.SetSkeletalAnimation(MeshManager::GetInstance().LoadSkeletalAnimation(filepath));
				}
			}

			ImGui::EndDragDropTarget();
		}

		float speed = skeletalAnimator.GetSpeed();
		if (ImGui::SliderFloat("Animation Speed", &speed, 0.0f, 5.0f))
		{
			skeletalAnimator.SetSpeed(speed);
		}

		if (skeletalAnimator.GetSkeletalAnimation())
		{
			float currentTime = skeletalAnimator.GetCurrentTime();
			if (ImGui::SliderFloat("Animation Current Time", &currentTime, 0.0f, skeletalAnimator.GetSkeletalAnimation()->GetDuration()))
			{
				skeletalAnimator.SetCurrentTime(currentTime);
			}
		}
	}
}

void Editor::CanvasComponent(const std::shared_ptr<Entity>& entity)
{
	if (!entity->HasComponent<Canvas>())
	{
		return;
	}

	Canvas& canvas = entity->GetComponent<Canvas>();

	ImGui::PushID("Canvas X");
	if (ImGui::Button("X"))
	{
		entity->RemoveComponent<Canvas>();
	}
	ImGui::PopID();

	ImGui::SameLine();

	if (ImGui::CollapsingHeader("Canvas"))
	{
		Indent indent;

		ImGui::Checkbox("Draw In Main Viewport", &canvas.drawInMainViewport);
		DrawIVec2Control("Size", canvas.size, 0.0f, { 0, 1024 });

		if (ImGui::BeginMenu("Scripts"))
		{
			for (const auto& [name, script] : ClayScriptManager::GetInstance().scriptsByName)
			{
				if (ImGui::MenuItem(name.c_str()))
				{
					canvas.script = script;
					canvas.scriptName = name;
				}
			}

			ImGui::EndMenu();
		}
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

	const bool previousOpened = opened;

	if (opened && ImGui::Begin("Material", &opened))
	{
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

		bool isChangedToSerialize = false;

		auto optionsByName = material->GetOptionsByName();
		if (!optionsByName.empty() && ImGui::CollapsingHeader("Options"))
		{
			for (auto& [name, option] : optionsByName)
			{
				if (ImGui::Checkbox(name.c_str(), &option.m_IsEnabled))
				{
					material->SetOption(name, option.m_IsEnabled);

					isChangedToSerialize = true;
				}
			}
		}

		for (const auto& [passName, pipeline] : material->GetBaseMaterial()->GetPipelinesByPass())
		{
			if (ImGui::CollapsingHeader(passName.c_str()))
			{
				for (const auto& [set, uniformLayout] : pipeline->GetUniformLayouts())
				{
					const auto& descriptorSetIndex = pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::MATERIAL, passName);
					if (!descriptorSetIndex || descriptorSetIndex.value() != set)
					{
						continue;
					}

					const std::shared_ptr<UniformWriter> uniformWriter = material->GetUniformWriter(passName);

					for (const auto& binding : uniformLayout->GetBindings())
					{
						if (binding.type == ShaderReflection::Type::COMBINED_IMAGE_SAMPLER)
						{
							if (std::shared_ptr<Texture> texture = uniformWriter->GetTexture(binding.name).back())
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
											material->GetUniformWriter(passName)->WriteTexture(binding.name, TextureManager::GetInstance().Load(filepath));
											
											isChangedToSerialize = true;
										}
									}

									ImGui::EndDragDropTarget();
								}

								ImGui::SameLine();

								const std::string whiteId = "Set White" + binding.name;
								ImGui::PushID(whiteId.c_str());
								if (ImGui::Button("White"))
								{
									material->GetUniformWriter(passName)->WriteTexture(binding.name, TextureManager::GetInstance().GetWhite());

									isChangedToSerialize = true;
								}
								ImGui::PopID();

								ImGui::SameLine();

								const std::string blackId = "Set Black" + binding.name;
								ImGui::PushID(blackId.c_str());
								if (ImGui::Button("Black"))
								{
									material->GetUniformWriter(passName)->WriteTexture(binding.name, TextureManager::GetInstance().GetBlack());

									isChangedToSerialize = true;
								}
								ImGui::PopID();

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
								// Variables with names that contain any "color" part and types vec3 or vec4 will be considered as a color variable
								// and will be represented in the editor as color pickers.
								if (Utils::Contains(Utils::ToLower(variable.name), "color"))
								{
									if (variable.type == ShaderReflection::ReflectVariable::Type::VEC3)
									{
										isChanged += ImGui::ColorEdit3(variable.name.c_str(), &Utils::GetValue<glm::vec3>(data, variable.offset)[0]);
									}
									if (variable.type == ShaderReflection::ReflectVariable::Type::VEC4)
									{
										isChanged += ImGui::ColorEdit4(variable.name.c_str(), &Utils::GetValue<glm::vec4>(data, variable.offset)[0]);
									}

									return;
								}

								// Variable with names that contain any "color" part and types int will be considered as a bool variable
								// and will be represented in the editor as check boxes.
								if (Utils::Contains(Utils::ToLower(variable.name), "use"))
								{
									if (variable.type == ShaderReflection::ReflectVariable::Type::INT)
									{
										isChanged += ImGui::Checkbox(variable.name.c_str(), &Utils::GetValue<bool>(data, variable.offset));
									}
									return;
								}

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

								isChangedToSerialize = true;
							}
						}
					}
				}
			}
		}

		if (isChangedToSerialize)
		{
			Material::Save(material, false);
		}

		ImGui::End();
	}

	if (previousOpened && !opened)
	{
		MaterialManager::GetInstance().DeleteMaterial(material);
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
		ImGui::Text("Are you sure you want to delete\n%s?", filepath.filename().string().c_str());
		if (ImGui::Button("Yes"))
		{
			if (Utils::GetFileFormat(filepath.string()).empty())
			{
				std::filesystem::remove_all(filepath);
			}
			else
			{
				std::filesystem::remove(filepath);

				const std::filesystem::path metaFilepath = filepath.concat(FileFormats::Meta());
				if (std::filesystem::exists(metaFilepath))
				{
					std::filesystem::remove(metaFilepath);
				}
			}

			// TODO: Do it more optimal, fine for now, very slow.
			filepathByUuid.clear();
			uuidByFilepath.clear();
			Serializer::GenerateFilesUUID(std::filesystem::current_path());

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

void Editor::CreateViewportMenu::Update(const Editor& editor, Window& window)
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
				auto callback = [this, &window]()
				{
					if (const std::shared_ptr<Viewport> viewport = window.GetViewportManager().GetViewport(name))
					{
						return;
					}

					window.GetViewportManager().Create(name, size);
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

void Editor::LoadIntermediateMenu::Update()
{
	if (!opened)
	{
		return;
	}

	ImGui::SetNextWindowSize({ 450.0f, 70.0f });
	ImGui::SetNextWindowPos(ImGui::GetWindowViewport()->GetCenter(), 0, { 0.5f, 0.0f });
	if (ImGui::Begin("Load Intermediate", nullptr,
		ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::Text(std::string("Work Name: " + workName).c_str());
		ImGui::ProgressBar(workStatus, ImVec2(ImGui::GetFontSize() * 25, 0.0f));
		ImGui::End();
	}
}

void Editor::TextureMetaPropertiesMenu::Update()
{
	if (!opened)
	{
		return;
	}

	if (opened && ImGui::Begin("Texture Meta Properties", &opened))
	{
		bool isChangedToSerialize = false;
		ImGui::Text("Filepath: %s", meta.filepath.string().c_str());
		isChangedToSerialize += ImGui::Checkbox("Create Mip Maps", &meta.createMipMaps);
		isChangedToSerialize += ImGui::Checkbox("SRGB", &meta.srgb);
		
		if (isChangedToSerialize)
		{
			Serializer::SerializeTextureMeta(meta);
		}

		ImGui::End();
	}
}

void Editor::ImportMenu::Update(Editor& editor)
{
	if (!opened)
	{
		return;
	}

	if (opened && ImGui::Begin("Import Properties", &opened))
	{
		ImGui::Checkbox("Meshes", &importMeshes);

		bool preTransformVertices = importFlags & aiProcess_PreTransformVertices;
		if (importMeshes && ImGui::Checkbox("PreTransform Vertices", &preTransformVertices))
		{
			importFlags = preTransformVertices ? importFlags | aiProcess_PreTransformVertices : importFlags & ~aiProcess_PreTransformVertices;
			importFlags = importFlags & ~aiProcess_PopulateArmatureData;
		}

		bool animations = importFlags & aiProcess_PopulateArmatureData;
		if (importMeshes && !preTransformVertices && ImGui::Checkbox("Animations", &animations))
		{
			importFlags = animations ? importFlags | aiProcess_PopulateArmatureData : importFlags & ~aiProcess_PopulateArmatureData;
		}

		if (importMeshes && !preTransformVertices)
		{
			ImGui::Checkbox("Skeletons", &importSkeletons);
		}

		ImGui::Checkbox("Materials", &importMaterials);

		if (ImGui::Button("Import"))
		{
			opened = false;

			editor.m_LoadIntermediateMenu.opened = true;
			editor.m_LoadIntermediateMenu.workStatus = 0;
			editor.m_LoadIntermediateMenu.workName.clear();

			ThreadPool::GetInstance().EnqueueAsync([&editor, this]()
			{
				filepath = Utils::Erase(filepath.string(), editor.m_RootDirectory.string() + "/");
				Serializer::LoadIntermediate(
					filepath,
					importMeshes,
					importMaterials,
					importSkeletons,
					importFlags,
					editor.m_LoadIntermediateMenu.workName,
					editor.m_LoadIntermediateMenu.workStatus);

				editor.m_LoadIntermediateMenu.opened = false;
			});
		}

		ImGui::End();
	}
}

void Editor::Thumbnails::Initialize()
{
	const std::string name = "Thumbnail";

	m_ThumbnailRenderer = Renderer::Create();
	m_ThumbnailScene = SceneManager::GetInstance().Create(name, name);
	m_ThumbnailWindow = Window::CreateHeadless(name, name, { 256, 256 });

	//m_ThumbnailScene->GetSettings().m_DrawBoundingBoxes = true;

	{
		auto entity = m_ThumbnailScene->CreateEntity("Sun");
		auto& transform = entity->AddComponent<Transform>(entity);
		transform.Rotate(glm::radians(glm::vec3(120.0f, -40.0f, 0.0f)));

		entity->AddComponent<DirectionalLight>().intensity = 5.0f;
	}

	auto camera = m_ThumbnailScene->CreateEntity("Camera");
	auto& cameraComponent =	camera->AddComponent<Camera>(camera);
	{
		m_CameraUUID = camera->GetUUID();
		cameraComponent.CreateRenderView(name, m_ThumbnailWindow->GetSize());
		camera->AddComponent<Transform>(camera);
	}

	{
		auto entity = m_ThumbnailScene->CreateEntity("Entity");
		entity->AddComponent<Transform>(entity);
		auto& r3d = entity->AddComponent<Renderer3D>();
		r3d.mesh = MeshManager::GetInstance().LoadMesh(std::filesystem::path("Meshes") / "Sphere.mesh");
		r3d.material = MaterialManager::GetInstance().LoadMaterial(std::filesystem::path("Materials") / "MeshBaseDoubleSided.mat");
	}

	const uint32_t previousSwapChainImageIndex = Vk::swapChainImageIndex;

	// Need to render n times to initialize every render target and etc.
	for (size_t i = 0; i < Vk::swapChainImageCount; i++)
	{
		// SetCamera and other functions in Renderer::Update send callbacks to create render target
		// and other resources on the next frame,
		// but we need it now, so we explicitly process events now.
		EventSystem::GetInstance().ProcessEvents();

		std::map<std::shared_ptr<Scene>, std::vector<Renderer::RenderViewportInfo>> viewportsByScene;
		const std::shared_ptr<Scene> scene = camera->GetScene();

		Renderer::RenderViewportInfo renderViewportInfo{};
		renderViewportInfo.camera = camera;
		renderViewportInfo.renderView = cameraComponent.GetRendererTarget(name);
		renderViewportInfo.size = m_ThumbnailWindow->GetSize();

		const float aspect = (float)renderViewportInfo.size.x / (float)renderViewportInfo.size.y;
		renderViewportInfo.projection = glm::perspective(cameraComponent.GetFov(), aspect, cameraComponent.GetZNear(), cameraComponent.GetZFar());

		viewportsByScene[scene].emplace_back(renderViewportInfo);

		void* frame = m_ThumbnailWindow->BeginFrame();

		m_ThumbnailRenderer->Update(frame, m_ThumbnailWindow, m_ThumbnailRenderer, viewportsByScene);

		m_ThumbnailWindow->EndFrame(frame);

		Vk::swapChainImageIndex = ++Vk::swapChainImageIndex % Vk::swapChainImageCount;
	}

	device->WaitIdle();

	Vk::swapChainImageIndex = previousSwapChainImageIndex;
}

void Editor::Thumbnails::UpdateThumbnails()
{
	//if (m_ThumbnailAtlas.GetAtlas(0))
	//{
	//	ImGui::Begin("Thumbnail Atlas");
	//	ImGui::Image((ImTextureID)m_ThumbnailAtlas.GetAtlas(0)->GetId(), ImVec2(1024, 1024));
	//	ImGui::End();
	//}

	if (m_ThumbnailToCheck == m_CacheThumbnails.end())
	{
		m_ThumbnailToCheck = m_CacheThumbnails.begin();
	}
	else
	{
		m_ThumbnailToCheck++;
	}

	if (m_ThumbnailQueue.empty())
	{
		return;
	}

	if (!std::filesystem::exists("Thumbnails"))
	{
		std::filesystem::create_directory("Thumbnails");
	}

	const ThumbnailLoadInfo thumbnailLoadInfo = m_ThumbnailQueue.front();
	m_ThumbnailQueue.pop_front();

	if (thumbnailLoadInfo.type == Type::MAT || thumbnailLoadInfo.type == Type::MESH)
	{
		UpdateMatMeshThumbnail(thumbnailLoadInfo);
	}
	else if(thumbnailLoadInfo.type == Type::SCENE || thumbnailLoadInfo.type == Type::PREFAB)
	{
		UpdateScenePrefabThumbnail(thumbnailLoadInfo);
	}
	else if (thumbnailLoadInfo.type == Type::TEXTURE)
	{
		UpdateTextureThumbnail(thumbnailLoadInfo);
	}
}

void Editor::Thumbnails::UpdateMatMeshThumbnail(const ThumbnailLoadInfo& thumbnailLoadInfo)
{
	const std::string name = "Thumbnail";

	std::map<std::shared_ptr<Scene>, std::vector<Renderer::RenderViewportInfo>> viewportsByScene;
	const std::shared_ptr<Entity> camera = m_ThumbnailScene->FindEntityByUUID(m_CameraUUID);
	Transform& cameraTransform = camera->GetComponent<Transform>();
	Camera& cameraComponent = camera->GetComponent<Camera>();
	const std::shared_ptr<Scene> scene = camera->GetScene();

	std::shared_ptr<Material> material = nullptr;
	std::shared_ptr<Mesh> mesh = nullptr;
	if (thumbnailLoadInfo.type == Type::MAT)
	{
		material = MaterialManager::GetInstance().LoadMaterial(thumbnailLoadInfo.resourceFilepath);

		// Check if material is skinned, then render default sphere, because no information about the mesh.
		if (Utils::Contains(Utils::ToLower(material->GetBaseMaterial()->GetName()), "skinned"))
		{
			material = nullptr;
		}

		cameraTransform.Translate({ 0.0f, 0.0f, 2.0f });
		cameraTransform.Rotate({});
		cameraComponent.SetZNear(100.0f * 0.001f);
		cameraComponent.SetZFar(100.0f);
	}
	else if (thumbnailLoadInfo.type == Type::MESH)
	{
		mesh = MeshManager::GetInstance().LoadMesh(thumbnailLoadInfo.resourceFilepath);

		BoundingBox bb = mesh->GetBoundingBox();

		glm::vec3 max = bb.max - bb.offset;
		glm::vec3 min = bb.offset - bb.min;

		float maxDistance = 0.0f;
		for (size_t i = 0; i < 3; i++)
		{
			maxDistance = glm::max<float>(maxDistance, max[i]);
			maxDistance = glm::max<float>(maxDistance, glm::abs<float>(min[i]));
		}

		const float distanceScale = 1.3f;
		glm::vec3 cameraPosition = glm::vec3(maxDistance * distanceScale) + bb.offset;
		const glm::mat4 transformMat4 = glm::inverse(glm::lookAt(cameraPosition, bb.offset, glm::vec3(0.0f, 1.0f, 0.0f)));
		glm::vec3 cameraRotation;
		Utils::DecomposeRotation(transformMat4, cameraRotation);

		cameraTransform.Translate(cameraPosition);
		cameraTransform.Rotate({ cameraRotation.x, cameraRotation.y, 0.0f });

		cameraComponent.SetZNear(maxDistance * distanceScale * 3.0f * 0.001f);
		cameraComponent.SetZFar(maxDistance * distanceScale * 3.0f);
	}

	if (scene != m_ThumbnailScene)
	{
		FATAL_ERROR("Scene for thumbnail is invalid, it is different from default editor thumbnail scene!");
	}

	const std::shared_ptr<Entity> entity = scene->FindEntityByName("Entity");
	auto& r3d = entity->GetComponent<Renderer3D>();

	r3d.mesh = mesh;
	r3d.material = material;

	if (!r3d.mesh)
	{
		r3d.mesh = MeshManager::GetInstance().LoadMesh(std::filesystem::path("Meshes") / "Sphere.mesh");
	}

	if (!r3d.material)
	{
		if (r3d.mesh && r3d.mesh->GetType() == Mesh::Type::SKINNED)
		{
			r3d.material = MaterialManager::GetInstance().LoadMaterial(std::filesystem::path("Materials") / "MeshBaseSkinned.mat");

			entity->AddComponent<SkeletalAnimator>();
		}
		else
		{
			r3d.material = MaterialManager::GetInstance().LoadMaterial(std::filesystem::path("Materials") / "MeshBaseDoubleSided.mat");
		}
	}

	Renderer::RenderViewportInfo renderViewportInfo{};
	renderViewportInfo.camera = camera;
	renderViewportInfo.renderView = cameraComponent.GetRendererTarget(name);
	renderViewportInfo.size = m_ThumbnailWindow->GetSize();

	const float aspect = (float)renderViewportInfo.size.x / (float)renderViewportInfo.size.y;
	renderViewportInfo.projection = glm::perspective(cameraComponent.GetFov(), aspect, cameraComponent.GetZNear(), cameraComponent.GetZFar());

	viewportsByScene[scene].emplace_back(renderViewportInfo);

	void* frame = m_ThumbnailWindow->BeginFrame();

	m_ThumbnailRenderer->Update(frame, m_ThumbnailWindow, m_ThumbnailRenderer, viewportsByScene);

	m_ThumbnailWindow->EndFrame(frame);

	device->WaitIdle();

	cameraComponent.TakeScreenshot(thumbnailLoadInfo.thumbnailFilepath, name, &m_GeneratingThumbnails.at(thumbnailLoadInfo.resourceFilepath));
	
	r3d.mesh = nullptr;
	r3d.material = nullptr;

	if (mesh)
	{
		if (mesh->GetType() == Mesh::Type::SKINNED)
		{
			entity->RemoveComponent<SkeletalAnimator>();
		}

		MeshManager::GetInstance().DeleteMesh(mesh);
	}

	if (material)
	{
		MaterialManager::GetInstance().DeleteMaterial(material);
	}
}

void Editor::Thumbnails::UpdateScenePrefabThumbnail(const ThumbnailLoadInfo& thumbnailLoadInfo)
{
	const std::string name = "Thumbnail";

	std::shared_ptr<Scene> scene = nullptr;
	std::shared_ptr<Entity> prefab = nullptr;

	if (thumbnailLoadInfo.type == Type::SCENE)
	{
		scene = thumbnailLoadInfo.scene;
		if (!scene)
		{
			Logger::Error("Can't generate scene thumbnail. Scene " + thumbnailLoadInfo.resourceFilepath.string() + " is nullptr!");
		}
	}
	else if (thumbnailLoadInfo.type == Type::PREFAB)
	{
		scene = m_ThumbnailScene;
		prefab = Serializer::DeserializePrefab(thumbnailLoadInfo.resourceFilepath, scene);
		prefab->GetComponent<Transform>().Translate({ 0.0f, 0.0f, 0.0f });
	}

	// Wait until all resources are loaded.
	AsyncAssetLoader::GetInstance().WaitIdle();

	auto camera = scene->CreateEntity("Camera");
	auto& cameraTransform = camera->AddComponent<Transform>(camera);
	auto& cameraComponent = camera->AddComponent<Camera>(camera);
	cameraComponent.CreateRenderView(name, m_ThumbnailWindow->GetSize());

	BoundingBox bb{};

	for (const auto& entity : scene->GetEntities())
	{
		if (!entity->HasComponent<Renderer3D>())
		{
			continue;
		}

		auto& r3d = entity->GetComponent<Renderer3D>();
		if (!r3d.mesh)
		{
			continue;
		}

		auto& transform = entity->GetComponent<Transform>();
		const glm::mat4 transformMat4 = transform.GetTransform();

		glm::vec3 max = transformMat4 * glm::vec4(r3d.mesh->GetBoundingBox().max, 1.0f);
		glm::vec3 min = transformMat4 * glm::vec4(r3d.mesh->GetBoundingBox().min, 1.0f);

		bb.max.x = glm::max(bb.max.x, max.x);
		bb.max.y = glm::max(bb.max.y, max.y);
		bb.max.z = glm::max(bb.max.z, max.z);

		bb.min.x = glm::min(bb.min.x, min.x);
		bb.min.y = glm::min(bb.min.y, min.y);
		bb.min.z = glm::min(bb.min.z, min.z);
	}

	bb.offset = bb.max + (bb.min - bb.max) * 0.5f;

	{
		glm::vec3 max = bb.max - bb.offset;
		glm::vec3 min = bb.offset - bb.min;

		float maxDistance = 0.0f;
		for (size_t i = 0; i < 3; i++)
		{
			maxDistance = glm::max<float>(maxDistance, max[i]);
			maxDistance = glm::max<float>(maxDistance, glm::abs<float>(min[i]));
		}

		const float distanceScale = 1.3f;
		glm::vec3 cameraPosition = glm::vec3(maxDistance * distanceScale) + bb.offset;
		const glm::mat4 transformMat4 = glm::inverse(glm::lookAt(cameraPosition, bb.offset, glm::vec3(0.0f, 1.0f, 0.0f)));
		glm::vec3 cameraRotation;
		Utils::DecomposeRotation(transformMat4, cameraRotation);

		cameraTransform.Translate(cameraPosition);
		cameraTransform.Rotate({ cameraRotation.x, cameraRotation.y, 0.0f });

		cameraComponent.SetZNear(maxDistance * distanceScale * 3.0f * 0.001f);
		cameraComponent.SetZFar(maxDistance * distanceScale * 3.0f);
	}

	uint32_t previousSwapChainImageIndex = Vk::swapChainImageIndex;

	// Need to render n times to initialize every render target and etc.
	// Though this is a scene thumbnail generation, which happen only on save scene action
	// and to this point everything should be already initialized,
	// but still let's just render a couple more frames.
	for (size_t i = 0; i < Vk::swapChainImageCount + 1; i++)
	{
		// SetCamera and other functions in Renderer::Update send callbacks to create render target
		// and other resources on the next frame,
		// but we need it now, so we explicitly process events now.
		EventSystem::GetInstance().ProcessEvents();

		std::map<std::shared_ptr<Scene>, std::vector<Renderer::RenderViewportInfo>> viewportsByScene;

		Renderer::RenderViewportInfo renderViewportInfo{};
		renderViewportInfo.camera = camera;
		renderViewportInfo.renderView = cameraComponent.GetRendererTarget(name);
		renderViewportInfo.size = m_ThumbnailWindow->GetSize();

		const float aspect = (float)renderViewportInfo.size.x / (float)renderViewportInfo.size.y;
		renderViewportInfo.projection = glm::perspective(cameraComponent.GetFov(), aspect, cameraComponent.GetZNear(), cameraComponent.GetZFar());

		viewportsByScene[scene].emplace_back(renderViewportInfo);

		void* frame = m_ThumbnailWindow->BeginFrame();

		m_ThumbnailRenderer->Update(frame, m_ThumbnailWindow, m_ThumbnailRenderer, viewportsByScene);

		m_ThumbnailWindow->EndFrame(frame);

		Vk::swapChainImageIndex = ++Vk::swapChainImageIndex % Vk::swapChainImageCount;
	}

	device->WaitIdle();

	Vk::swapChainImageIndex = previousSwapChainImageIndex;

	cameraComponent.TakeScreenshot(thumbnailLoadInfo.thumbnailFilepath, name, &m_GeneratingThumbnails.at(thumbnailLoadInfo.resourceFilepath));

	device->WaitIdle();

	scene->DeleteEntity(camera);

	if (prefab)
	{
		scene->DeleteEntity(prefab);
	}
}

void Editor::Thumbnails::UpdateTextureThumbnail(const ThumbnailLoadInfo& thumbnailLoadInfo)
{
	std::shared_ptr<Texture> srcTexture = TextureManager::GetInstance().Load(thumbnailLoadInfo.resourceFilepath);
	std::shared_ptr<Texture> dstTexture = RenderPassManager::GetInstance().ScaleTexture(srcTexture, { 64, 64 });
	Serializer::SerializeTexture(thumbnailLoadInfo.thumbnailFilepath, dstTexture, &m_GeneratingThumbnails.at(thumbnailLoadInfo.resourceFilepath));
	TextureManager::GetInstance().Delete(srcTexture);
}

ThumbnailAtlas::TileInfo Editor::Thumbnails::GetOrGenerateThumbnail(
	const std::filesystem::path& filepath,
	std::shared_ptr<Scene> scene,
	Type type)
{
	auto foundThumbnail = m_CacheThumbnails.find(filepath);
	if (m_ThumbnailToCheck != foundThumbnail && foundThumbnail != m_CacheThumbnails.end())
	{
		auto tileInfo = foundThumbnail->second;
		if (tileInfo.atlasIndex >= 0 && tileInfo.tileIndex >= 0)
		{
			return tileInfo;
		}
		else
		{
			m_CacheThumbnails.erase(foundThumbnail);
			m_ThumbnailToCheck = m_CacheThumbnails.end();
		}
	}

	auto shortFilepath = Utils::GetShortFilepath(filepath);
	const auto uuid = Utils::FindUuid(shortFilepath);
	if (uuid.IsValid())
	{
		const std::string uuidString = uuid.ToString();

		std::filesystem::path thumbnailFilepath = "Thumbnails";
		thumbnailFilepath /= uuidString;
		thumbnailFilepath.concat(FileFormats::Png());

		std::filesystem::path thumbnailMetaFilepath = "Thumbnails";
		thumbnailMetaFilepath /= uuidString;
		thumbnailMetaFilepath.concat(FileFormats::Png());
		thumbnailMetaFilepath.concat(".thumbnail");

		size_t lastWriteTimeResourceCurrent = 0;
		size_t lastWriteTimeResourceSaved = 1;

		if (std::filesystem::exists(filepath))
		{
			lastWriteTimeResourceCurrent = std::filesystem::last_write_time(filepath).time_since_epoch().count();
		}

		if (std::filesystem::exists(thumbnailMetaFilepath))
		{
			lastWriteTimeResourceSaved = Serializer::DeserializeThumbnailMeta(thumbnailMetaFilepath);
		}

		auto found = m_GeneratingThumbnails.find(shortFilepath);
		if (found != m_GeneratingThumbnails.end())
		{
			if (!found->second)
			{
				return {};
			}

			TextureManager::GetInstance().Delete(thumbnailFilepath);
			m_GeneratingThumbnails.erase(shortFilepath);
		}

		if (lastWriteTimeResourceSaved == lastWriteTimeResourceCurrent && std::filesystem::exists(thumbnailFilepath))
		{
			auto& tileInfo = m_CacheThumbnails[filepath];
			if (tileInfo.atlasIndex >= 0 && tileInfo.tileIndex >= 0)
			{
				return tileInfo;
			}

			if (!m_IsThumbnailLoading.load())
			{
				m_IsThumbnailLoading.store(true);
				AsyncAssetLoader::GetInstance().AsyncLoadTexture(thumbnailFilepath, [this, filepath](std::weak_ptr<Texture> texture)
				{
					ThumbnailAtlas::TileInfo tileInfo{};
					const auto atlas = m_ThumbnailAtlas.Push(texture.lock(), tileInfo);
					m_CacheThumbnails[filepath] = tileInfo;
					m_IsThumbnailLoading.store(false);

					auto callback = [texture]()
					{
						TextureManager::GetInstance().Delete(texture.lock()->GetFilepath());
					};

					std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
					EventSystem::GetInstance().SendEvent(event);
				}, false);
			}
		}
		else if (!m_GeneratingThumbnails.contains(shortFilepath))
		{
			auto& tileInfo = m_CacheThumbnails[filepath];
			m_ThumbnailAtlas.Free(tileInfo.atlasIndex, tileInfo.tileIndex);
			tileInfo = {};

			m_GeneratingThumbnails.emplace(shortFilepath, false);

			ThumbnailLoadInfo thumbnailLoadInfo{};
			thumbnailLoadInfo.type = type;
			thumbnailLoadInfo.scene = scene;
			thumbnailLoadInfo.resourceFilepath = shortFilepath;
			thumbnailLoadInfo.thumbnailFilepath = thumbnailFilepath;
			m_ThumbnailQueue.emplace_back(thumbnailLoadInfo);

			Serializer::SerializeThumbnailMeta(thumbnailMetaFilepath, lastWriteTimeResourceCurrent);
		}
	}

	return {};
}

ThumbnailAtlas::TileInfo Editor::Thumbnails::TryGetThumbnail(
	const std::filesystem::path& filepath)
{
	auto foundThumbnail = m_CacheThumbnails.find(filepath);
	if (m_ThumbnailToCheck != foundThumbnail && foundThumbnail != m_CacheThumbnails.end())
	{
		auto tileInfo = foundThumbnail->second;
		if (tileInfo.atlasIndex >= 0 && tileInfo.tileIndex >= 0)
		{
			return tileInfo;
		}
		else
		{
			m_CacheThumbnails.erase(foundThumbnail);
			m_ThumbnailToCheck = m_CacheThumbnails.end();
		}
	}

	auto shortFilepath = Utils::GetShortFilepath(filepath);
	const auto uuid = Utils::FindUuid(shortFilepath);
	if (uuid.IsValid())
	{
		const std::string uuidString = uuid.ToString();

		std::filesystem::path thumbnailFilepath = "Thumbnails";
		thumbnailFilepath /= uuidString;
		thumbnailFilepath.concat(FileFormats::Png());

		auto found = m_GeneratingThumbnails.find(shortFilepath);
		if (found != m_GeneratingThumbnails.end())
		{
			if (!found->second)
			{
				return {};
			}

			TextureManager::GetInstance().Delete(thumbnailFilepath);
			m_GeneratingThumbnails.erase(shortFilepath);
		}

		if (std::filesystem::exists(thumbnailFilepath))
		{
			auto& tileInfo = m_CacheThumbnails[filepath];
			if (tileInfo.atlasIndex >= 0 && tileInfo.tileIndex >= 0)
			{
				return tileInfo;
			}

			if (!m_IsThumbnailLoading.load())
			{
				m_IsThumbnailLoading.store(true);
				AsyncAssetLoader::GetInstance().AsyncLoadTexture(thumbnailFilepath, [this, filepath](std::weak_ptr<Texture> texture)
				{
					ThumbnailAtlas::TileInfo tileInfo{};
					const auto atlas = m_ThumbnailAtlas.Push(texture.lock(), tileInfo);
					m_CacheThumbnails[filepath] = tileInfo;
					m_IsThumbnailLoading.store(false);

					auto callback = [texture]()
					{
						TextureManager::GetInstance().Delete(texture.lock()->GetFilepath());
					};

					std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
					EventSystem::GetInstance().SendEvent(event);
				}, false);
			}
		}
	}

	return {};
}
