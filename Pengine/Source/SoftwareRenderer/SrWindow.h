#pragma once

#include "../Core/Window.h"

#include <Windows.h>

namespace Pengine
{

	class PENGINE_API SrWindow : public Window
	{
	public:
		SrWindow(const std::wstring& name, const glm::ivec2& size);
		~SrWindow();
		SrWindow(const SrWindow&) = delete;
		SrWindow& operator=(const SrWindow&) = delete;

		virtual void Update() override;

		virtual void Resize(const glm::ivec2& size) override;

		virtual void NewFrame() override;

		virtual void EndFrame() override;

		virtual void Clear(const glm::vec4& color) override;

		virtual void Present(std::shared_ptr<Texture> texture) override;

		virtual void ShutDownPrepare() override;

	private:

		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void ProcessMessages();

		glm::vec2 m_ScreenPixelsInBitMapPixels = { 1.0f, 1.0f };

		HINSTANCE m_HInstance{};

		HWND m_HWnd{};

		BITMAPINFO m_BitMapInfo{};

		HBITMAP m_HBitMap{};

		BLENDFUNCTION m_BlendFunction{};

		void* m_BitMapMemory = nullptr;

		int channels = 4;
	};

}