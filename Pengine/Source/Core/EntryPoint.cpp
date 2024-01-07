#include "EntryPoint.h"

#include "Scene.h"
#include "Input.h"
#include "Logger.h"
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
#include "../EventSystem/EventSystem.h"
#include "../Graphics/Renderer.h"
#include "../Editor/Editor.h"

using namespace Pengine;

EntryPoint::EntryPoint(Application* application)
	: m_Application(application)
{
	EngineConfig engineConfig = Serializer::DeserializeEngineConfig("Configs/Engine.yaml");
	graphicsAPI = engineConfig.graphicsAPI;
}

void EntryPoint::Run()
{
	EventSystem& eventSystem = EventSystem::GetInstance();
	std::shared_ptr<Window> mainWindow = WindowManager::GetInstance().Create("Pengine",
		{ 800, 800 });
	WindowManager::GetInstance().SetCurrentWindow(mainWindow);

	Editor editor;

	RenderPassManager::GetInstance();
	m_Application->OnPreStart();

	std::shared_ptr<Viewport> viewport = ViewportManager::GetInstance().Create("Main", { 800, 800 });
	Serializer::GenerateFilesUUID(std::filesystem::current_path());

	TextureManager::GetInstance().LoadFromFolder("Editor/Images");

	m_Application->OnStart();

	while (mainWindow->IsRunning())
	{
		Time::GetInstance().Update();

		for (auto window : WindowManager::GetInstance().GetWindows())
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
			for (const auto& viewport : ViewportManager::GetInstance().GetViewports())
			{
				std::shared_ptr<Entity> camera = viewport.second->GetCamera();
				if (camera && camera->HasComponent<Camera>())
				{
					viewport.second->Update(
						camera->GetComponent<Camera>().GetRenderer()->GetRenderPassFrameBuffer(Deferred)->GetAttachment(0));
				}
				else
				{
					viewport.second->Update(TextureManager::GetInstance().GetWhite());
				}
			}

			editor.Update(SceneManager::GetInstance().GetSceneByTag("Main"));

			window->ImGuiEnd();

			if (void* frame = window->BeginFrame())
			{
				for (const auto& viewport : ViewportManager::GetInstance().GetViewports())
				{
					if (std::shared_ptr<Entity> camera = viewport.second->GetCamera())
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
							viewport.second->SetCamera({});
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
