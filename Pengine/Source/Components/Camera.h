#pragma once

#include "../Core/Core.h"
#include "../Core/Entity.h"

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

		explicit Camera(std::shared_ptr<Entity> entity);
		Camera(const Camera& camera);
		Camera(Camera&& camera) noexcept;
		Camera& operator=(const Camera& camera);
		Camera& operator=(Camera&& camera) noexcept;

		[[nodiscard]] glm::mat4 GetViewProjectionMat4() const { return m_ViewProjectionMat4; }

		[[nodiscard]] glm::mat4 GetProjectionMat4() const { return m_ProjectionMat4; }

		[[nodiscard]] glm::mat4 GetViewMat4() const { return m_ViewMat4; }

		[[nodiscard]] glm::vec2 GetSize() const { return m_Size; }

		void SetSize(const glm::vec2& size);

		[[nodiscard]] Type GetType() const { return m_Type; }

		void SetOrthographic(const glm::ivec2& size);

		void SetPerspective(const glm::ivec2& size);

		void SetType(Type type);

		void UpdateProjection();

		[[nodiscard]] float GetAspect() const { return static_cast<float>(m_Size.x) / static_cast<float>(m_Size.y); }

		[[nodiscard]] float GetFov() const { return m_Fov; }

		void SetFov(float fov);

		[[nodiscard]] float GetZNear() const { return m_Znear; }

		void SetZNear(float zNear);

		[[nodiscard]] float GetZFar() const { return m_Zfar; }

		void SetZFar(float zFar);

		[[nodiscard]] std::shared_ptr<Renderer> GetRenderer() const { return m_Renderer; }

		[[nodiscard]] std::shared_ptr<Entity> GetEntity() const { return m_Entity; }

		[[nodiscard]] const std::string& GetViewport() const { return m_Viewport; }

		void SetViewport(const std::string& viewport) { m_Viewport = viewport; }

	private:
		glm::mat4 m_ViewMat4{};
		glm::mat4 m_ProjectionMat4{};
		glm::mat4 m_ViewProjectionMat4{};

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
