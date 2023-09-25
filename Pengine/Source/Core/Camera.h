#pragma once

#include "Core.h"
#include "../Components/Transform.h"

namespace Pengine
{

	class PENGINE_API Camera
	{
	public:
		enum class CameraType
		{
			ORTHOGRAPHIC,
			PERSPECTIVE
		};

		Transform m_Transform;

		Camera();
		Camera(const Camera& camera);
		void operator=(const Camera& camera);

		glm::mat4 GetViewProjectionMat4() const { return m_ViewProjectionMat4; }

		glm::mat4 GetProjectionMat4() const { return m_ProjectionMat4; }

		glm::mat4 GetViewMat4() const { return m_ViewMat4; }

		glm::vec2 GetSize() const { return m_Size; }

		void SetSize(const glm::vec2& size);

		float GetPitch() const { return m_Transform.GetRotation().x; }

		float GetYaw() const { return m_Transform.GetRotation().y; }

		float GetRoll() const { return m_Transform.GetRotation().z; }

		CameraType GetType() const { return m_Type; }

		void SetOrthographic(const glm::ivec2& size);

		void SetPerspective(const glm::ivec2& size);

		void SetType(CameraType type);

		void SetScene(std::shared_ptr<class Scene> scene) { m_Scene = scene; }

		void UpdateProjection();

		float GetAspect() const { return (float)m_Size.x / (float)m_Size.y; }

		float GetFov() const { return m_Fov; }

		void SetFov(float fov);

		float GetZNear() const { return m_Znear; }

		void SetZNear(float zNear);

		float GetZFar() const { return m_Zfar; }

		void SetZFar(float zFar);

		std::shared_ptr<class Renderer> GetRenderer() const { return m_Renderer; }

		std::shared_ptr<class Scene> GetScene() const { return m_Scene; }

	private:
		glm::mat4 m_ViewMat4;
		glm::mat4 m_ProjectionMat4;
		glm::mat4 m_ViewProjectionMat4;

		std::shared_ptr<class Scene> m_Scene;
		std::shared_ptr<class Renderer> m_Renderer;

		glm::ivec2 m_Size = { 0, 0 };

		float m_Znear = 0.1f;
		float m_Zfar = 1000.0f;
		float m_Fov = glm::radians(90.0f);

		CameraType m_Type = CameraType::ORTHOGRAPHIC;

		void Copy(const Camera& camera);

		void UpdateViewProjection();
	};

}