#include "EditorApplication.h"

#include "Core/SceneManager.h"
#include "Core/WindowManager.h"
#include "Core/Logger.h"
#include "Core/Serializer.h"
#include "Core/Raycast.h"
#include "Graphics/Device.h"
#include "Components/Transform.h"
#include "Components/Camera.h"

void EditorApplication::OnStart()
{
	ImGui::SetCurrentContext(Pengine::WindowManager::GetInstance().GetCurrentWindow()->GetImGuiContext());
	m_Editor = std::make_unique<Editor>();
}

void EditorApplication::OnUpdate()
{
}

void EditorApplication::OnImGuiUpdate()
{
	m_Editor->Update(Pengine::SceneManager::GetInstance().GetSceneByTag("Main"), *Pengine::WindowManager::GetInstance().GetCurrentWindow().get());
}

void EditorApplication::OnClose()
{
	m_Editor = nullptr;
}
