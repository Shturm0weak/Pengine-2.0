#pragma once

#include "../Core/Core.h"
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

		std::shared_ptr<class Camera> GetCamera() const { return m_Camera; }

		void SetCamera(std::shared_ptr<class Camera> camera);

		glm::ivec2 GetSize() const { return m_Size; }

		bool IsHovered() const { return m_IsHovered; }

		bool IsFocused() const { return m_IsFocused; }

	private:
		void Resize(const glm::ivec2& size);

		glm::ivec2 m_Size;
		
		std::string m_Name;

		std::shared_ptr<class Camera> m_Camera;

		bool m_IsHovered = false;
		bool m_IsFocused = false;
	};

}