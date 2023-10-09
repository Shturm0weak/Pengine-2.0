#include "ExampleApplication.h"

#include "Core/Input.h"
#include "Core/Time.h"
#include "Core/MaterialManager.h"
#include "Core/MeshManager.h"
#include "Core/RenderPassManager.h"
#include "Core/Serializer.h"
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

	std::shared_ptr<Scene> scene = SceneManager::GetInstance().Create("Scene", "Main");

	std::shared_ptr<Camera> camera = std::make_shared<Camera>();
	camera->SetType(Camera::CameraType::PERSPECTIVE);
	camera->SetScene(scene);
	ViewportManager::GetInstance().GetViewport("Main")->SetCamera(camera);

	camera->m_Transform.Rotate(glm::vec3(glm::radians(30.0f), 0.0f, 0.0f));
	camera->m_Transform.Translate(glm::vec3(0.0f, 0.0f, 2.0f));

	//TODO: Rework filepath system, use std::path!

	Logger::Log("Test ecs");

	GameObjectC& gameobject0 = scene->CreateGameObjectC();
	Entity entity = gameobject0.GetEntity().Clone();

	bool hasGameObject = entity.HasComponent<GameObjectC>();
	GameObjectC& gameobject1 = entity.GetComponent<GameObjectC>();
	scene->DeleteEntity(gameobject0.GetEntity());
	scene->DeleteEntity(entity);
}

void ExampleApplication::OnUpdate()
{
	std::shared_ptr<Viewport> viewport = ViewportManager::GetInstance().GetViewport("Main");
	if (!viewport->IsFocused())
	{
		return;
	}

	const float speed = 2.0f;
	std::shared_ptr<Camera> camera = viewport->GetCamera();
	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_W))
	{
		camera->m_Transform.Translate(camera->m_Transform.GetPosition() + camera->m_Transform.GetForward() * (float)Time::GetDeltaTime() * speed);
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::KEY_S))
	{
		camera->m_Transform.Translate(camera->m_Transform.GetPosition() + camera->m_Transform.GetForward() * -(float)Time::GetDeltaTime() * speed);
	}
	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_D))
	{
		camera->m_Transform.Translate(camera->m_Transform.GetPosition() + camera->m_Transform.GetRight() * (float)Time::GetDeltaTime() * speed);
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::KEY_A))
	{
		camera->m_Transform.Translate(camera->m_Transform.GetPosition() + camera->m_Transform.GetRight() * -(float)Time::GetDeltaTime() * speed);
	}

	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_LEFT_CONTROL))
	{
		camera->m_Transform.Translate(camera->m_Transform.GetPosition() + camera->m_Transform.GetUp() * -(float)Time::GetDeltaTime() * speed);
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::SPACE))
	{
		camera->m_Transform.Translate(camera->m_Transform.GetPosition() + camera->m_Transform.GetUp() * (float)Time::GetDeltaTime() * speed);
	}

	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_UP))
	{
		camera->m_Transform.Rotate(camera->m_Transform.GetRotation() + glm::vec3(1.0f * Time::GetDeltaTime() * speed, 0.0f, 0.0f));
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::KEY_DOWN))
	{
		camera->m_Transform.Rotate(camera->m_Transform.GetRotation() + glm::vec3(-1.0f * Time::GetDeltaTime() * speed, 0.0f, 0.0f));
	}
	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_RIGHT))
	{
		camera->m_Transform.Rotate(camera->m_Transform.GetRotation() + glm::vec3(0.0f, -1.0f * Time::GetDeltaTime() * speed, 0.0f));
	}
	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_LEFT))
	{
		camera->m_Transform.Rotate(camera->m_Transform.GetRotation() + glm::vec3(0.0f, 1.0f * Time::GetDeltaTime() * speed, 0.0f));
	}
}

void ExampleApplication::OnClose()
{
}
