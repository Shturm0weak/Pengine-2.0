#include "ExampleApplication.h"

#include "Core/Input.h"
#include "Core/Time.h"
#include "Core/MaterialManager.h"
#include "Core/MeshManager.h"
#include "Core/RenderPassManager.h"
#include "Components/Camera.h"
#include "Components/Renderer3D.h"

using namespace Pengine;

void ExampleApplication::OnPreStart()
{
}

void ExampleApplication::OnStart()
{
	Texture::CreateInfo whiteTextureCreateInfo;
	whiteTextureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	whiteTextureCreateInfo.channels = 4;
	whiteTextureCreateInfo.filepath = "White";
	whiteTextureCreateInfo.name = "White";
	whiteTextureCreateInfo.format = Texture::Format::R8G8B8A8_SRGB;
	whiteTextureCreateInfo.size = { 1, 1 };
	whiteTextureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_DST };
	std::vector<uint8_t> pixels = {
		255,
		255,
		255,
		255
	};
	whiteTextureCreateInfo.data = pixels;
	TextureManager::GetInstance().Create(whiteTextureCreateInfo);

	std::shared_ptr<Scene> scene = SceneManager::GetInstance().Create("Default", "Main");

	std::shared_ptr<Entity> camera = scene->CreateEntity("Camera");
	Transform& transform = camera->AddComponent<Transform>(camera);
	Camera& cameraComponent = camera->AddComponent<Camera>(camera);

	cameraComponent.SetType(Camera::Type::PERSPECTIVE);
	ViewportManager::GetInstance().GetViewport("Main")->SetCamera(camera);

	transform.Rotate(glm::vec3(glm::radians(30.0f), 0.0f, 0.0f));
	transform.Translate(glm::vec3(0.0f, 0.0f, 2.0f));
}

void ExampleApplication::OnUpdate()
{
}

void ExampleApplication::OnClose()
{
}