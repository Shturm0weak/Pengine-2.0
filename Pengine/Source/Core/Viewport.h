#pragma once

#include "Core.h"
#include "Entity.h"

#include "../Graphics/Texture.h"

namespace Pengine
{

	class PENGINE_API Viewport : public std::enable_shared_from_this<Viewport>
	{
	public:
		Viewport(std::string name, const glm::ivec2& size);
		virtual ~Viewport() = default;
		Viewport(const Viewport&) = delete;
		Viewport& operator=(const Viewport&) = delete;

		void Update(const std::shared_ptr<Texture>& viewportTexture, std::shared_ptr<class Window> window);

		[[nodiscard]] std::weak_ptr<Entity> GetCamera() const { return m_Camera; }

		void SetCamera(const std::shared_ptr<Entity>& camera);

		void SetIsOpened(bool isOpened) { m_IsOpened = isOpened; }

		void SetIsHeadLess(bool isHeadLess) { m_IsHeadLess = isHeadLess; }

		bool IsHeadLess() const { return m_IsHeadLess; }

		[[nodiscard]] glm::ivec2 GetSize() const { return m_Size; }

		[[nodiscard]] glm::vec2 GetPosition() const { return m_Position; }

		[[nodiscard]] glm::vec2 GetMousePosition() const { return m_MousePosition; }

		[[nodiscard]] glm::vec3 GetMouseRay(const glm::vec2& mousePosition) const;

		[[nodiscard]] const glm::mat4& GetProjectionMat4() const { return m_Projection; }

		[[nodiscard]] bool IsHovered() const { return m_IsHovered; }

		[[nodiscard]] bool IsFocused() const { return m_IsFocused; }

		[[nodiscard]] const std::string& GetName() const { return m_Name; }

		[[nodiscard]] uint32_t& GetGizmoOperation() { return m_GizmoOperation; }

		void SetDrawGizmosCallback(const std::function<void(const glm::vec2&, glm::ivec2, std::shared_ptr<Entity>, bool&)>& drawGizmosCallback);

		void UpdateProjectionMat4();

	private:
		void Resize(const glm::ivec2& size);

		void SetOrthographic(const glm::ivec2& size, const float zFar, const float zNear);

		void SetPerspective(const glm::ivec2& size, const float zFar, const float zNear, const float fov);

		glm::mat4 m_Projection{};

		std::function<void(const glm::vec2&, glm::ivec2, std::shared_ptr<Entity>, bool&)> m_DrawGizmosCallback;

		glm::vec2 m_MousePosition;
		glm::vec2 m_Position{};
		glm::ivec2 m_Size{};
		
		std::string m_Name;

		std::weak_ptr<Entity> m_Camera;

		uint32_t m_GizmoOperation = 0;

		bool m_IsHovered = false;
		bool m_IsFocused = false;
		bool m_IsOpened = true;
		bool m_ActiveGuizmo = false;

		bool m_IsHeadLess = false;
	};

}
