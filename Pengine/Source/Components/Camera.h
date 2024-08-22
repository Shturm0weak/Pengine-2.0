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

		void SetEntity(std::shared_ptr<Entity> entity);

		[[nodiscard]] glm::mat4 GetViewMat4() const { return m_ViewMat4; }

		[[nodiscard]] Type GetType() const { return m_Type; }

		void SetType(Type type);

		void CreateRenderTarget(const std::string& name, const glm::ivec2& size);

		void ResizeRenderTarget(const std::string& name, const glm::ivec2& size);

		void DeleteRenderTarget(const std::string& name);

		[[nodiscard]] float GetFov() const { return m_Fov; }

		void SetFov(float fov);

		[[nodiscard]] float GetZNear() const { return m_Znear; }

		void SetZNear(float zNear);

		[[nodiscard]] float GetZFar() const { return m_Zfar; }

		void SetZFar(float zFar);

		[[nodiscard]] std::shared_ptr<Renderer> GetRendererTarget(const std::string& name) const;

		[[nodiscard]] std::shared_ptr<Entity> GetEntity() const { return m_Entity; }

	private:
		glm::mat4 m_ViewMat4{};

		std::unordered_map<std::string, std::shared_ptr<Renderer>> m_RenderersByName;

		std::shared_ptr<Entity> m_Entity;

		float m_Znear = 0.1f;
		float m_Zfar = 1000.0f;
		float m_Fov = glm::radians(90.0f);

		Type m_Type = Type::ORTHOGRAPHIC;

		void Copy(const Camera& camera);

		void Move(Camera&& camera) noexcept;

		void UpdateViewMat4();
	};

}
