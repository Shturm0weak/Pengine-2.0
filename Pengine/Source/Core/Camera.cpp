#include "Camera.h"

#include "Window.h"
#include "Viewport.h"
#include "Logger.h"
#include "../EventSystem/EventSystem.h"
#include "../EventSystem/NextFrameEvent.h"
#include "../EventSystem/ResizeEvent.h"
#include "../Graphics/Renderer.h"

using namespace Pengine;

void Camera::Copy(const Camera& camera)
{
	m_ProjectionMat4 = camera.m_ProjectionMat4;
	m_ViewProjectionMat4 = camera.m_ViewProjectionMat4;
	m_Fov = camera.m_Fov;
	m_Type = camera.m_Type;
	m_Transform = camera.m_Transform;
}

void Camera::UpdateViewProjection()
{
	m_ViewMat4 = glm::inverse(m_Transform.GetTransform());
	m_ViewProjectionMat4 = m_ProjectionMat4 * m_ViewMat4;
}

Camera::Camera()
{
	m_Renderer = Renderer::Create(m_Size);

	m_Transform.SetOnRotationCallback("Camera", std::bind(&Camera::UpdateViewProjection, this));
	m_Transform.SetOnTranslationCallback("Camera", std::bind(&Camera::UpdateViewProjection, this));
	SetType(CameraType::ORTHOGRAPHIC);
}

Camera::Camera(const Camera& camera)
{
	Copy(camera);
}

void Camera::operator=(const Camera& camera)
{
	Copy(camera);
}

void Camera::SetSize(const glm::vec2& size)
{
	m_Size = size;

	if (m_Renderer)
	{
		auto callback = [this]()
		{
			m_Renderer->Resize(m_Size);
		};

		NextFrameEvent* event = new NextFrameEvent(callback, Event::Type::OnNextFrame, this);
		EventSystem::GetInstance().SendEvent(event);
	}

	UpdateProjection();
}

void Camera::SetOrthographic(const glm::ivec2& size)
{
	if (size.x == 0 || size.y == 0)
	{
		m_Size = { 1, 1 };
	}
	else
	{
		m_Size = size;
	}

	const float ratio = GetAspect();
	m_ProjectionMat4 = glm::ortho(-ratio, ratio, -1.0f,
		1.0f, m_Znear, m_Zfar);

	UpdateViewProjection();
}

void Camera::SetPerspective(const glm::ivec2& size)
{
	if (size.x == 0 || size.y == 0)
	{
		m_Size = { 1, 1 };
	}
	else
	{
		m_Size = size;
	}

	m_ProjectionMat4 = glm::perspective(m_Fov, GetAspect(), m_Znear, m_Zfar);

	UpdateViewProjection();
}

void Camera::SetType(CameraType type)
{
	m_Type = type;

	UpdateProjection();
}

void Camera::UpdateProjection()
{
	switch (m_Type)
	{
	case CameraType::PERSPECTIVE:
	{
		SetPerspective(m_Size);
		break;
	}
	case CameraType::ORTHOGRAPHIC:
	default:
	{
		SetOrthographic(m_Size);
		break;
	}
	}
}

void Camera::SetFov(float fov)
{
	m_Fov = fov;

	UpdateProjection();
}

void Camera::SetZNear(float zNear)
{
	m_Znear = zNear;

	UpdateProjection();
}

void Camera::SetZFar(float zFar)
{
	m_Zfar = zFar;

	UpdateProjection();
}
