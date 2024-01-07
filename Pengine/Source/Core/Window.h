#pragma once

#include "Core.h"

#include "../Graphics/Texture.h"

namespace Pengine
{

	class PENGINE_API Window
	{
	public:
		Window(const std::string& name, const glm::ivec2& size);
		virtual ~Window() = default;
		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		virtual void Update() = 0;

		virtual bool Resize(const glm::ivec2& size);

		virtual void NewFrame() = 0;

		virtual void EndFrame() = 0;

		virtual void Clear(const glm::vec4& color) = 0;

		virtual void Present(std::shared_ptr<Texture> texture) = 0;

		virtual void ShutDownPrepare() = 0;

		virtual void ImGuiBegin() = 0;

		virtual void ImGuiEnd() = 0;

		virtual void* BeginFrame() = 0;

		virtual void EndFrame(void* frame) = 0;

		virtual void ImGuiRenderPass(void* frame) = 0;

		virtual void DisableCursor() = 0;

		virtual void ShowCursor() = 0;

		virtual void HideCursor() = 0;

		void SetIsRunning(bool isRunning) { m_IsRunning = isRunning; };

		bool IsRunning() const { return m_IsRunning; }

		bool IsMinimized() const { return m_IsMinimized; }

		glm::ivec2 GetSize() const { return m_Size; }

	protected:
		glm::ivec2 m_Size = { 0, 0 };

		std::string m_Name;

	private:
		bool m_IsRunning = true;
		bool m_IsMinimized = false;
	};

}