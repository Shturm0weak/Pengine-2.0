#include "EntryPoint.h"

#include "Input.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "RenderPassManager.h"
#include "SceneManager.h"
#include "Serializer.h"
#include "TextureManager.h"
#include "Time.h"
#include "ViewportManager.h"
#include "WindowManager.h"

#include "../Components/Camera.h"
#include "../Editor/Editor.h"
#include "../EventSystem/EventSystem.h"
#include "../Graphics/Renderer.h"

using namespace Pengine;

EntryPoint::EntryPoint(Application* application)
	: m_Application(application)
{
	const auto [configGraphicsAPI] = Serializer::DeserializeEngineConfig("Configs/Engine.yaml");
	graphicsAPI = configGraphicsAPI;
}

void EntryPoint::Run() const
{
	EventSystem& eventSystem = EventSystem::GetInstance();
	const std::shared_ptr<Window> mainWindow = WindowManager::GetInstance().Create("Pengine",
		{ 800, 800 });
	WindowManager::GetInstance().SetCurrentWindow(mainWindow);

	Editor editor;

	RenderPassManager::GetInstance();
	m_Application->OnPreStart();

	ViewportManager::GetInstance().Create("Main", { 800, 800 });
	Serializer::GenerateFilesUUID(std::filesystem::current_path());

	TextureManager::GetInstance().LoadFromFolder("Editor/Images");

	m_Application->OnStart();

	while (mainWindow->IsRunning())
	{
		Time::GetInstance().Update();

		for (const auto& window : WindowManager::GetInstance().GetWindows())
		{
			if (!window->IsRunning())
			{
				WindowManager::GetInstance().Destroy(window);
				continue;
			}

			WindowManager::GetInstance().SetCurrentWindow(window);
			window->NewFrame();

			if (window->IsMinimized())
			{
				continue;
			}

			m_Application->OnUpdate();

			window->ImGuiBegin();
			for (const auto& [name, viewport] : ViewportManager::GetInstance().GetViewports())
			{
				if (const std::shared_ptr<Entity> camera = viewport->GetCamera();
					camera && camera->HasComponent<Camera>())
				{
					viewport->Update(
						camera->GetComponent<Camera>().GetRenderer()->GetRenderPassFrameBuffer(
							Deferred)->GetAttachment(0));
				}
				else
				{
					viewport->Update(TextureManager::GetInstance().GetWhite());
				}
			}

			editor.Update(SceneManager::GetInstance().GetSceneByTag("Main"));

			window->ImGuiEnd();

			if (void* frame = window->BeginFrame())
			{
				for (const auto& [name, viewport] : ViewportManager::GetInstance().GetViewports())
				{
					if (const std::shared_ptr<Entity> camera = viewport->GetCamera())
					{
						if (camera->HasComponent<Camera>())
						{
							Camera& cameraComponent = camera->GetComponent<Camera>();
							cameraComponent.GetRenderer()->Update(
								frame,
								window,
								camera->GetScene(),
								camera);
						}
						else
						{
							viewport->SetCamera({});
						}
					}
				}

				window->ImGuiRenderPass(frame);
				window->EndFrame(frame);
			}

			eventSystem.ProcessEvents();
		}
	}

	mainWindow->ShutDownPrepare();

	m_Application->OnClose();

	MaterialManager::GetInstance().ShutDown();
	MeshManager::GetInstance().ShutDown();
	SceneManager::GetInstance().ShutDown();
	TextureManager::GetInstance().ShutDown();
	RenderPassManager::GetInstance().ShutDown();
	ViewportManager::GetInstance().ShutDown();
	WindowManager::GetInstance().ShutDown();
}
