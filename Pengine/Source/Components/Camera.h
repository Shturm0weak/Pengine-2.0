#pragma once

#include "../Core/Core.h"
#include "../Core/Entity.h"
#include "../Components/Transform.h"

namespace Pengine
{

	class Renderer;

	class PENGINE_API Camera
	{
	public:
		enum class Type
		{
			ORTHOGRAPHIC,
			PERSPECTIVE
		};

		Camera(std::shared_ptr<Entity> entity);
		Camera(const Camera& camera);
		Camera(Camera&& camera) noexcept;
		void operator=(const Camera& camera);
		void operator=(Camera&& camera) noexcept;

		glm::mat4 GetViewProjectionMat4() const { return m_ViewProjectionMat4; }

		glm::mat4 GetProjectionMat4() const { return m_ProjectionMat4; }

		glm::mat4 GetViewMat4() const { return m_ViewMat4; }

		glm::vec2 GetSize() const { return m_Size; }

		void SetSize(const glm::vec2& size);

		Type GetType() const { return m_Type; }

		void SetOrthographic(const glm::ivec2& size);

		void SetPerspective(const glm::ivec2& size);

		void SetType(Type type);

		void UpdateProjection();

		float GetAspect() const { return (float)m_Size.x / (float)m_Size.y; }

		float GetFov() const { return m_Fov; }

		void SetFov(float fov);

		float GetZNear() const { return m_Znear; }

		void SetZNear(float zNear);

		float GetZFar() const { return m_Zfar; }

		void SetZFar(float zFar);

		std::shared_ptr<Renderer> GetRenderer() const { return m_Renderer; }

		std::shared_ptr<Entity> GetEntity() { return m_Entity; }

		const std::string& GetViewport() const { return m_Viewport; }

		void SetViewport(const std::string& viewport) { m_Viewport = viewport; }

	private:
		glm::mat4 m_ViewMat4;
		glm::mat4 m_ProjectionMat4;
		glm::mat4 m_ViewProjectionMat4;

		std::shared_ptr<Renderer> m_Renderer;

		glm::ivec2 m_Size = { 0, 0 };

		std::shared_ptr<Entity> m_Entity;

		float m_Znear = 0.1f;
		float m_Zfar = 1000.0f;
		float m_Fov = glm::radians(90.0f);

		Type m_Type = Type::ORTHOGRAPHIC;

		std::string m_Viewport = none;

		void Copy(const Camera& camera);

		void Move(Camera&& camera) noexcept;

		void UpdateViewProjection();
	};

}