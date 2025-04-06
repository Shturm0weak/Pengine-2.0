#include "Camera.h"

#include "Transform.h"

#include "../Core/RenderPassOrder.h"
#include "../Core/Serializer.h"
#include "../Core/FileFormatNames.h"
#include "../EventSystem/EventSystem.h"
#include "../EventSystem/NextFrameEvent.h"
#include "../Graphics/RenderTarget.h"
#include "../Utils/Utils.h"

using namespace Pengine;

void Camera::Copy(const Camera& camera)
{
	m_ViewMat4 = camera.GetViewMat4();
	m_Fov = camera.GetFov();
	m_Type = camera.GetType();
	m_Zfar = camera.GetZFar();
	m_Znear = camera.GetZNear();
}

void Camera::Move(Camera&& camera) noexcept
{
	m_ViewMat4 = camera.m_ViewMat4;
	m_Fov = camera.GetFov();
	m_Type = camera.GetType();
	m_Zfar = camera.GetZFar();
	m_Znear = camera.GetZNear();
}

void Camera::UpdateViewMat4()
{
	// Potential bad performance impact! Maybe need to reinvent!
	if (m_Entity && m_Entity->HasComponent<Transform>())
	{
		m_ViewMat4 = glm::inverse(m_Entity->GetComponent<Transform>().GetTransform());
	}
}

Camera::Camera(std::shared_ptr<Entity> entity)
	: m_Entity(std::move(entity))
{
	if (m_Entity && m_Entity->HasComponent<Transform>())
	{
		Transform& transform = m_Entity->GetComponent<Transform>();
		transform.SetOnRotationCallback("Camera", std::bind(&Camera::UpdateViewMat4, this));
		transform.SetOnTranslationCallback("Camera", std::bind(&Camera::UpdateViewMat4, this));
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

void Camera::SetEntity(std::shared_ptr<Entity> entity)
{
	m_Entity = entity;

	if (m_Entity && m_Entity->HasComponent<Transform>())
	{
		Transform& transform = m_Entity->GetComponent<Transform>();
		transform.SetOnRotationCallback("Camera", std::bind(&Camera::UpdateViewMat4, this));
		transform.SetOnTranslationCallback("Camera", std::bind(&Camera::UpdateViewMat4, this));
	}

	SetType(Type::PERSPECTIVE);
}

void Camera::SetType(const Type type)
{
	m_Type = type;

	UpdateViewMat4();
}

void Camera::CreateRenderTarget(const std::string& name, const glm::ivec2& size)
{
	auto callback = [this, name, size]()
	{
		m_RenderTargetsByName[name] = RenderTarget::Create(passPerViewportOrder, size);
	};

	std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
	EventSystem::GetInstance().SendEvent(event);
}

void Camera::ResizeRenderTarget(const std::string& name, const glm::ivec2& size)
{
	if (std::shared_ptr<RenderTarget> renderTarget = GetRendererTarget(name))
	{
		auto callback = [weakRenderTarget = std::weak_ptr<RenderTarget>(renderTarget), size]()
		{
			if (std::shared_ptr<RenderTarget> renderTarget = weakRenderTarget.lock())
			{
				renderTarget->Resize(size);
			}
		};

		std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
		EventSystem::GetInstance().SendEvent(event);
	}
}

void Camera::DeleteRenderTarget(const std::string& name)
{
	if (std::shared_ptr<RenderTarget> renderTarget = GetRendererTarget(name))
	{
		auto callback = [weakrenderTarget = std::weak_ptr<RenderTarget>(renderTarget), name, this]()
		{
			if (std::shared_ptr<RenderTarget> renderTarget = weakrenderTarget.lock())
			{
				renderTarget = nullptr;
				m_RenderTargetsByName[name] = nullptr;
			}
		};

		std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
		EventSystem::GetInstance().SendEvent(event);
	}
}

void Camera::SetFov(const float fov)
{
	m_Fov = fov;

	UpdateViewMat4();
}

void Camera::SetZNear(const float zNear)
{
	m_Znear = zNear;

	UpdateViewMat4();
}

void Camera::SetZFar(const float zFar)
{
	m_Zfar = zFar;

	UpdateViewMat4();
}

std::shared_ptr<RenderTarget> Camera::GetRendererTarget(const std::string& name) const
{
	return Utils::Find(name, m_RenderTargetsByName);
}

void Camera::TakeScreenshot(const std::filesystem::path& filepath, const std::string& viewportName, bool* isLoaded)
{
	const std::shared_ptr<RenderTarget> renderTarget = GetRendererTarget(viewportName);
	if (!renderTarget)
	{
		return;
	}

	const std::shared_ptr<FrameBuffer> frameBuffer = renderTarget->GetFrameBuffer(GetPassName());
	if (!frameBuffer)
	{
		return;
	}

	const std::shared_ptr<Texture> texture = frameBuffer->GetAttachment(GetRenderTargetIndex());
	if (!texture)
	{
		return;
	}

	Serializer::SerializeTexture(filepath, texture, isLoaded);
}
