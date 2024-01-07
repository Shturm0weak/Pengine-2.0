#pragma once

#include "Core.h"
#include "Entity.h"
#include "../Graphics/Texture.h"

namespace Pengine
{

	class PENGINE_API Viewport
	{
	public:
		Viewport(const std::string& name, const glm::ivec2& size);
		virtual ~Viewport() = default;
		Viewport(const Viewport&) = delete;
		Viewport& operator=(const Viewport&) = delete;

		void Update(std::shared_ptr<Texture> viewportTexture);

		std::shared_ptr<Entity> GetCamera() const { return m_Camera; }

		void SetCamera(std::shared_ptr<Entity> camera);

		glm::ivec2 GetSize() const { return m_Size; }

		glm::vec2 GetPosition() const { return m_Position; }

		bool IsHovered() const { return m_IsHovered; }

		bool IsFocused() const { return m_IsFocused; }

		void SetDrawGizmosCallback(const std::function<void(const glm::vec2& position, const glm::ivec2 size, std::shared_ptr<Entity> camera)>& drawGizmosCallback);

	private:
		void Resize(const glm::ivec2& size);

		std::function<void(const glm::vec2& position, const glm::ivec2 size, std::shared_ptr<Entity> camera)> m_DrawGizmosCallback;

		glm::vec2 m_Position;
		glm::ivec2 m_Size;
		
		std::string m_Name;

		std::shared_ptr<Entity> m_Camera;

		bool m_IsHovered = false;
		bool m_IsFocused = false;
	};

}