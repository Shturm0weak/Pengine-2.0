#include "ExampleApplication.h"

#include "Core/Input.h"
#include "Core/Time.h"
#include "Core/MaterialManager.h"
#include "Core/MeshManager.h"
#include "Core/RenderPassManager.h"
#include "Components/Renderer3D.h"

using namespace Pengine;

void ExampleApplication::OnPreStart()
{
}

void ExampleApplication::OnStart()
{
	Texture::CreateInfo whiteTexturecreateInfo;
	whiteTexturecreateInfo.aspectMask = Texture::AspectMask::COLOR;
	whiteTexturecreateInfo.channels = 4;
	whiteTexturecreateInfo.filepath = "White";
	whiteTexturecreateInfo.name = "White";
	whiteTexturecreateInfo.format = Texture::Format::R8G8B8A8_SRGB;
	whiteTexturecreateInfo.size = { 1, 1 };
	whiteTexturecreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_DST };
	uint8_t* pixels = new uint8_t[4];
	pixels[0] = 255;
	pixels[1] = 255;
	pixels[2] = 255;
	pixels[3] = 255;
	whiteTexturecreateInfo.data = pixels;
	TextureManager::GetInstance().Create(whiteTexturecreateInfo);

	std::shared_ptr<Scene> scene = SceneManager::GetInstance().Create("Scene", "Main");

	std::shared_ptr<Camera> camera = std::make_shared<Camera>();
	camera->SetType(Camera::CameraType::PERSPECTIVE);
	camera->SetScene(scene);
	ViewportManager::GetInstance().GetViewport("Main")->SetCamera(camera);

	camera->m_Transform.Rotate(glm::vec3(glm::radians(30.0f), 0.0f, 0.0f));
	camera->m_Transform.Translate(glm::vec3(0.0f, 0.0f, 2.0f));

	MaterialManager::GetInstance().LoadBaseMaterial("Materials/MeshBase.basemat");
}

void ExampleApplication::OnUpdate()
{
	std::shared_ptr<Viewport> viewport = ViewportManager::GetInstance().GetViewport("Main");
	if (!viewport->IsFocused())
	{
		return;
	}

	std::shared_ptr<Camera> camera = viewport->GetCamera();
	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_W))
	{
		camera->m_Transform.Translate(camera->m_Transform.GetPosition() + camera->m_Transform.GetForward() * (float)Time::GetDeltaTime());
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::KEY_S))
	{
		camera->m_Transform.Translate(camera->m_Transform.GetPosition() + camera->m_Transform.GetForward() * -(float)Time::GetDeltaTime());
	}
	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_D))
	{
		camera->m_Transform.Translate(camera->m_Transform.GetPosition() + camera->m_Transform.GetRight() * (float)Time::GetDeltaTime());
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::KEY_A))
	{
		camera->m_Transform.Translate(camera->m_Transform.GetPosition() + camera->m_Transform.GetRight() * -(float)Time::GetDeltaTime());
	}

	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_LEFT_CONTROL))
	{
		camera->m_Transform.Translate(camera->m_Transform.GetPosition() + camera->m_Transform.GetUp() * -(float)Time::GetDeltaTime());
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::SPACE))
	{
		camera->m_Transform.Translate(camera->m_Transform.GetPosition() + camera->m_Transform.GetUp() * (float)Time::GetDeltaTime());
	}

	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_UP))
	{
		camera->m_Transform.Rotate(camera->m_Transform.GetRotation() + glm::vec3(1.0f * Time::GetDeltaTime(), 0.0f, 0.0f));
	}
	else if (Input::KeyBoard::IsKeyDown(Keycode::KEY_DOWN))
	{
		camera->m_Transform.Rotate(camera->m_Transform.GetRotation() + glm::vec3(-1.0f * Time::GetDeltaTime(), 0.0f, 0.0f));
	}
	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_RIGHT))
	{
		camera->m_Transform.Rotate(camera->m_Transform.GetRotation() + glm::vec3(0.0f, -1.0f * Time::GetDeltaTime(), 0.0f));
	}
	if (Input::KeyBoard::IsKeyDown(Keycode::KEY_LEFT))
	{
		camera->m_Transform.Rotate(camera->m_Transform.GetRotation() + glm::vec3(0.0f, 1.0f * Time::GetDeltaTime(), 0.0f));
	}
}

void ExampleApplication::OnClose()
{
}
