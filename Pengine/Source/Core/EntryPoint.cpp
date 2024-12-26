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

void PrepareResources()
{
	Texture::CreateInfo whiteTextureCreateInfo;
	whiteTextureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	whiteTextureCreateInfo.channels = 4;
	whiteTextureCreateInfo.filepath = "White";
	whiteTextureCreateInfo.name = "White";
	whiteTextureCreateInfo.format = Format::R8G8B8A8_SRGB;
	whiteTextureCreateInfo.size = { 1, 1 };
	whiteTextureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_DST };
	std::vector<uint8_t> whitePixels = {
		255,
		255,
		255,
		255
	};
	whiteTextureCreateInfo.data = whitePixels.data();
	TextureManager::GetInstance().Create(whiteTextureCreateInfo);

	Texture::CreateInfo blackTextureCreateInfo;
	blackTextureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	blackTextureCreateInfo.channels = 4;
	blackTextureCreateInfo.filepath = "Black";
	blackTextureCreateInfo.name = "Black";
	blackTextureCreateInfo.format = Format::R8G8B8A8_SRGB;
	blackTextureCreateInfo.size = { 1, 1 };
	blackTextureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_DST };
	std::vector<uint8_t> blackPixels = {
		0,
		0,
		0,
		0
	};
	blackTextureCreateInfo.data = blackPixels.data();
	TextureManager::GetInstance().Create(blackTextureCreateInfo);

	{
		std::vector<float> vertices =
		{
			-1.0f, -1.0f,
			 1.0f, -1.0f,
			 1.0f,  1.0f,
			-1.0f,  1.0f
		};

		std::vector<uint32_t> indices =
		{
			0, 1, 2, 2, 3, 0
		};

		MeshManager::GetInstance().CreateMesh("FullScreenQuad", "FullScreenQuad", sizeof(glm::vec2), vertices, indices);
	}

	{
		std::vector<uint32_t> indices =
		{
			//Top
			2, 6, 7,
			2, 3, 7,

			//Bottom
			0, 4, 5,
			0, 1, 5,

			//Left
			0, 2, 6,
			0, 4, 6,

			//Right
			1, 3, 7,
			1, 5, 7,

			//Front
			0, 2, 3,
			0, 1, 3,

			//Back
			4, 6, 7,
			4, 5, 7
		};


		std::vector<float> vertices =
		{
			-1.0f, -1.0f,  1.0f, //0
			 1.0f, -1.0f,  1.0f, //1
			-1.0f,  1.0f,  1.0f, //2
			 1.0f,  1.0f,  1.0f, //3
			-1.0f, -1.0f, -1.0f, //4
			 1.0f, -1.0f, -1.0f, //5
			-1.0f,  1.0f, -1.0f, //6
			 1.0f,  1.0f, -1.0f  //7
		};

		MeshManager::GetInstance().CreateMesh("SkyBoxCube", "SkyBoxCube", sizeof(glm::vec3), vertices, indices);
	}
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

	PrepareResources();

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

		eventSystem.ProcessEvents();

		m_Application->OnUpdate();

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
						viewport->Update(renderTarget->GetFrameBuffer(cameraComponent.GetRenderPassName())->GetAttachment(cameraComponent.GetRenderTargetIndex()));
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
