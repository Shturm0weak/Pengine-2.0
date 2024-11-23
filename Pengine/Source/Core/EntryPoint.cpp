#include "EntryPoint.h"

#include "AsyncAssetLoader.h"
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
#include "ThreadPool.h"

#include "../Components/Camera.h"
#include "../Editor/Editor.h"
#include "../EventSystem/EventSystem.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderTarget.h"

using namespace Pengine;

EntryPoint::EntryPoint(Application* application)
	: m_Application(application)
{
	const auto [configGraphicsAPI] = Serializer::DeserializeEngineConfig("Configs\\Engine.yaml");
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

	TextureManager::GetInstance().LoadFromFolder("Editor\\Images");

	ThreadPool::GetInstance().Initialize();

	const std::shared_ptr<Renderer> renderer = Renderer::Create();

	m_Application->OnStart();

	while (mainWindow->IsRunning())
	{
		Time::GetInstance().Update();
		AsyncAssetLoader::GetInstance().Update();

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
					const std::shared_ptr<RenderTarget> renderTarget = cameraComponent.GetRendererTarget(name);
					if (!renderTarget)
					{
						continue;
					}

					if (!cameraComponent.GetRenderPassName().empty())
					{
						viewport->Update(renderTarget->GetRenderPassFrameBuffer(cameraComponent.GetRenderPassName())->GetAttachment(cameraComponent.GetRenderTargetIndex()));
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

			drawCallsCount = 0;
			vertexCount = 0;

			if (void* frame = window->BeginFrame())
			{
				std::map<std::shared_ptr<Scene>, std::vector<Renderer::RenderViewportInfo>> viewportsByScene;

				for (const auto& [name, viewport] : ViewportManager::GetInstance().GetViewports())
				{
					if (const std::shared_ptr<Entity> camera = viewport->GetCamera().lock())
					{
						if (camera->HasComponent<Camera>())
						{
							Renderer::RenderViewportInfo renderViewportInfo{};
							renderViewportInfo.camera = camera;
							renderViewportInfo.renderTarget = camera->GetComponent<Camera>().GetRendererTarget(name);
							renderViewportInfo.projection = viewport->GetProjectionMat4();
							renderViewportInfo.size = viewport->GetSize();

							viewportsByScene[camera->GetScene()].emplace_back(renderViewportInfo);
						}
					}
				}

				Renderer::Update(
					frame,
					window,
					renderer,
					viewportsByScene);

				window->ImGuiRenderPass(frame);
				window->EndFrame(frame);
			}

			eventSystem.ProcessEvents();
		}

		++currentFrame;
	}

	mainWindow->ShutDownPrepare();

	m_Application->OnClose();

	ThreadPool::GetInstance().Shutdown();
	SceneManager::GetInstance().ShutDown();
	MaterialManager::GetInstance().ShutDown();
	MeshManager::GetInstance().ShutDown();
	TextureManager::GetInstance().ShutDown();
	RenderPassManager::GetInstance().ShutDown();
	ViewportManager::GetInstance().ShutDown();
	WindowManager::GetInstance().ShutDown();
}
