#include "ExampleApplication.h"

#include "Core/Input.h"
#include "Core/Time.h"
#include "Core/MaterialManager.h"
#include "Core/MeshManager.h"
#include "Core/RenderPassManager.h"
#include "Core/Serializer.h"
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
	std::shared_ptr<Viewport> viewport = ViewportManager::GetInstance().GetViewport("Main");
	if (!viewport->IsFocused())
	{
		return;
	}

	const float speed = 2.0f;
	std::shared_ptr<Entity> camera = viewport->GetCamera();
	if (!camera)
	{
		return;
	}

	Camera& cameraComponent = camera->GetComponent<Camera>();
	Transform& transform = camera->GetComponent<Transform>();

	if (!viewport->IsFocused() || !Input::Mouse::IsMouseDown(Keycode::MOUSE_BUTTON_2))
	{
		return;
	}

	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_W))
	{
		transform.Translate(transform.GetPosition() + transform.GetForward() * (float)Time::GetDeltaTime() * speed);
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::KEY_S))
	{
		transform.Translate(transform.GetPosition() + transform.GetForward() * -(float)Time::GetDeltaTime() * speed);
	}
	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_D))
	{
		transform.Translate(transform.GetPosition() + transform.GetRight() * (float)Time::GetDeltaTime() * speed);
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::KEY_A))
	{
		transform.Translate(transform.GetPosition() + transform.GetRight() * -(float)Time::GetDeltaTime() * speed);
	}

	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_LEFT_CONTROL))
	{
		transform.Translate(transform.GetPosition() + transform.GetUp() * -(float)Time::GetDeltaTime() * speed);
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::SPACE))
	{
		transform.Translate(transform.GetPosition() + transform.GetUp() * (float)Time::GetDeltaTime() * speed);
	}

	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_UP))
	{
		transform.Rotate(transform.GetRotation() + glm::vec3(1.0f * Time::GetDeltaTime() * speed, 0.0f, 0.0f));
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::KEY_DOWN))
	{
		transform.Rotate(transform.GetRotation() + glm::vec3(-1.0f * Time::GetDeltaTime() * speed, 0.0f, 0.0f));
	}
	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_RIGHT))
	{
		transform.Rotate(transform.GetRotation() + glm::vec3(0.0f, -1.0f * Time::GetDeltaTime() * speed, 0.0f));
	}
	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_LEFT))
	{
		transform.Rotate(transform.GetRotation() + glm::vec3(0.0f, 1.0f * Time::GetDeltaTime() * speed, 0.0f));
	}
}

void ExampleApplication::OnClose()
{
}