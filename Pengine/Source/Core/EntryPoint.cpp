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
#include "Viewport.h"
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

#ifndef NO_EDITOR
	Editor editor;
#endif

	RenderPassManager::GetInstance();
	m_Application->OnPreStart();

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
				if (const std::shared_ptr<Entity> camera = viewport->GetCamera().lock();
					camera && camera->HasComponent<Camera>())
				{
					Camera& cameraComponent = camera->GetComponent<Camera>();
					const std::shared_ptr<Renderer> renderer = cameraComponent.GetRendererTarget(name);
					if (!renderer)
					{
						continue;
					}

					if (!cameraComponent.GetRenderPassName().empty())
					{
						viewport->Update(renderer->GetRenderPassFrameBuffer(cameraComponent.GetRenderPassName())->GetAttachment(cameraComponent.GetRenderTargetIndex()));
					}
					else
					{
						viewport->Update(TextureManager::GetInstance().GetWhite());
					}
				}
				else
				{
					viewport->Update(TextureManager::GetInstance().GetWhite());
				}
			}

#ifndef NO_EDITOR
			editor.Update(SceneManager::GetInstance().GetSceneByTag("Main"));
#endif
			window->ImGuiEnd();

			if (void* frame = window->BeginFrame())
			{
				for (const auto& [name, viewport] : ViewportManager::GetInstance().GetViewports())
				{
					if (const std::shared_ptr<Entity> camera = viewport->GetCamera().lock())
					{
						if (camera->HasComponent<Camera>())
						{
							Camera& cameraComponent = camera->GetComponent<Camera>();
							const std::shared_ptr<Renderer> renderer = cameraComponent.GetRendererTarget(name);
							if (!renderer)
							{
								continue;
							}
							renderer->Update(
								frame,
								window,
								camera->GetScene(),
								camera,
								viewport->GetProjectionMat4());
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

	SceneManager::GetInstance().ShutDown();
	MaterialManager::GetInstance().ShutDown();
	MeshManager::GetInstance().ShutDown();
	TextureManager::GetInstance().ShutDown();
	RenderPassManager::GetInstance().ShutDown();
	ViewportManager::GetInstance().ShutDown();
	WindowManager::GetInstance().ShutDown();
}
