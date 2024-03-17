#include "Camera.h"

#include "Transform.h"

#include "../EventSystem/EventSystem.h"
#include "../EventSystem/NextFrameEvent.h"
#include "../Graphics/Renderer.h"

using namespace Pengine;

void Camera::Copy(const Camera& camera)
{
	m_ViewMat4 = camera.GetViewMat4();
	m_ProjectionMat4 = camera.GetProjectionMat4();
	m_ViewProjectionMat4 = camera.GetViewProjectionMat4();
	m_Viewport = camera.GetViewport();
	m_Fov = camera.GetFov();
	m_Type = camera.GetType();
	m_Zfar = camera.GetZFar();
	m_Znear = camera.GetZNear();
	m_Size = camera.GetSize();
}

void Camera::Move(Camera&& camera) noexcept
{
	m_ViewMat4 = camera.m_ViewMat4;
	m_ProjectionMat4 = camera.m_ProjectionMat4;
	m_ViewProjectionMat4 = camera.m_ViewProjectionMat4;
	m_Fov = camera.GetFov();
	m_Type = camera.GetType();
	m_Zfar = camera.GetZFar();
	m_Znear = camera.GetZNear();
	m_Size = camera.GetSize();
}

void Camera::UpdateViewProjection()
{
	// Potential bad performance impact! Maybe need to reinvent!
	if (m_Entity && m_Entity->HasComponent<Transform>())
	{
		m_ViewMat4 = glm::inverse(m_Entity->GetComponent<Transform>().GetTransform());
	}
	m_ViewProjectionMat4 = m_ProjectionMat4 * m_ViewMat4;
}

Camera::Camera(std::shared_ptr<Entity> entity)
	: m_Entity(std::move(entity))
{
	m_Renderer = Renderer::Create(m_Size);

	if (m_Entity && m_Entity->HasComponent<Transform>())
	{
		Transform& transform = m_Entity->GetComponent<Transform>();
		transform.SetOnRotationCallback("Camera", std::bind(&Camera::UpdateViewProjection, this));
		transform.SetOnTranslationCallback("Camera", std::bind(&Camera::UpdateViewProjection, this));
	}

	SetType(Type::PERSPECTIVE);
}

Camera::Camera(const Camera& camera)
{
	Copy(camera);
}

Camera::Camera(Camera&& camera) noexcept
{
	Move(std::move(camera));
}

Camera& Camera::operator=(const Camera& camera)
{
	if (this != &camera)
	{
		Copy(camera);
	}

	return *this;
}

Camera& Camera::operator=(Camera&& camera) noexcept
{
	Move(std::move(camera));
	return *this;
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

void Camera::SetType(const Type type)
{
	m_Type = type;

	UpdateProjection();
}

void Camera::UpdateProjection()
{
	switch (m_Type)
	{
	case Type::PERSPECTIVE:
	{
		SetPerspective(m_Size);
		break;
	}
	case Type::ORTHOGRAPHIC:
	default:
	{
		SetOrthographic(m_Size);
		break;
	}
	}
}

void Camera::SetFov(const float fov)
{
	m_Fov = fov;

	UpdateProjection();
}

void Camera::SetZNear(const float zNear)
{
	m_Znear = zNear;

	UpdateProjection();
}

void Camera::SetZFar(const float zFar)
{
	m_Zfar = zFar;

	UpdateProjection();
}
