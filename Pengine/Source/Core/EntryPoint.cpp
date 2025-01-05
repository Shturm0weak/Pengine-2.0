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
	Texture::CreateInfo whiteTextureCreateInfo{};
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

	Texture::CreateInfo pinkTextureCreateInfo{};
	pinkTextureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	pinkTextureCreateInfo.channels = 4;
	pinkTextureCreateInfo.filepath = "Pink";
	pinkTextureCreateInfo.name = "Pink";
	pinkTextureCreateInfo.format = Format::R8G8B8A8_SRGB;
	pinkTextureCreateInfo.size = { 1, 1 };
	pinkTextureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_DST };
	std::vector<uint8_t> pinkPixels = {
		255,
		0,
		255,
		255
	};
	pinkTextureCreateInfo.data = pinkPixels.data();
	TextureManager::GetInstance().Create(pinkTextureCreateInfo);

	Texture::CreateInfo blackTextureCreateInfo{};
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

	Texture::CreateInfo whiteLayeredTextureCreateInfo;
	whiteLayeredTextureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	whiteLayeredTextureCreateInfo.channels = 4;
	whiteLayeredTextureCreateInfo.filepath = "WhiteLayered";
	whiteLayeredTextureCreateInfo.name = "WhiteLayered";
	whiteLayeredTextureCreateInfo.format = Format::R8G8B8A8_SRGB;
	whiteLayeredTextureCreateInfo.size = { 1, 1 };
	whiteLayeredTextureCreateInfo.layerCount = 2;;
	whiteLayeredTextureCreateInfo.data = nullptr;
	whiteLayeredTextureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::COLOR_ATTACHMENT, Texture::Usage::TRANSFER_DST };
	TextureManager::GetInstance().Create(whiteLayeredTextureCreateInfo);

	{
		glm::vec2* vertices = new glm::vec2[4];
		vertices[0] = { -1.0f, -1.0f };
		vertices[1] = {  1.0f, -1.0f };
		vertices[2] = {  1.0f,  1.0f };
		vertices[3] = { -1.0f,  1.0f };

		std::vector<uint32_t> indices =
		{
			0, 1, 2, 2, 3, 0
		};

		Mesh::CreateInfo createInfo{};
		createInfo.filepath = "FullScreenQuad";
		createInfo.name = "FullScreenQuad";
		createInfo.indices = std::move(indices);
		createInfo.vertices = vertices;
		createInfo.vertexSize = sizeof(glm::vec2);
		createInfo.vertexCount = 4;
		createInfo.vertexLayouts =
		{
			VertexLayout(createInfo.vertexSize)
		};

		MeshManager::GetInstance().CreateMesh(createInfo);
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

		glm::vec3* vertices = new glm::vec3[8];
		vertices[0] = { -1.0f, -1.0f,  1.0f };
		vertices[1] = {  1.0f, -1.0f,  1.0f };
		vertices[2] = { -1.0f,  1.0f,  1.0f };
		vertices[3] = {  1.0f,  1.0f,  1.0f };
		vertices[4] = { -1.0f, -1.0f, -1.0f };
		vertices[5] = {  1.0f, -1.0f, -1.0f };
		vertices[6] = { -1.0f,  1.0f, -1.0f };
		vertices[7] = {  1.0f,  1.0f, -1.0f };

		Mesh::CreateInfo createInfo{};
		createInfo.filepath = "SkyBoxCube";
		createInfo.name = "SkyBoxCube";
		createInfo.indices = std::move(indices);
		createInfo.vertices = vertices;
		createInfo.vertexSize = sizeof(glm::vec3);
		createInfo.vertexCount = 8;
		createInfo.vertexLayouts =
		{
			VertexLayout(createInfo.vertexSize)
		};

		MeshManager::GetInstance().CreateMesh(createInfo);
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
						const std::shared_ptr<FrameBuffer> frameBuffer = renderTarget->GetFrameBuffer(cameraComponent.GetRenderPassName());
						if (frameBuffer)
						{
							viewport->Update(frameBuffer->GetAttachment(cameraComponent.GetRenderTargetIndex()));
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
