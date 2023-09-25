#include "SrWindow.h"

#include "../Core/WindowManager.h"
#include "../SoftwareRenderer/SrTexture.h"

using namespace Pengine;

SrWindow::SrWindow(const std::wstring& name, const glm::ivec2& size)
	: Window(name, size)
{
    m_HInstance = GetModuleHandle(nullptr);
    m_ScreenPixelsInBitMapPixels = { 1.0f, 1.0f };
    const wchar_t* className = L"WINDOWCLASS";

    WNDCLASS wndClass{};
    wndClass.lpszClassName = className;
    wndClass.hInstance = m_HInstance;
    wndClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.style = CS_DBLCLKS;
    wndClass.lpfnWndProc = WindowProc;
    RegisterClass(&wndClass);

    DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

    RECT rect;
    rect.left = 500;
    rect.top = 250;
    rect.right = rect.left + size.x;
    rect.bottom = rect.top + size.y;

    AdjustWindowRect(&rect, style, false);

    m_HWnd = CreateWindowEx(
        0,
        className,
        m_Name.c_str(),
        style,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        m_HInstance,
        NULL
    );

    ShowWindow(m_HWnd, SW_SHOW);

    m_BlendFunction.BlendOp = AC_SRC_OVER;
    m_BlendFunction.BlendFlags = 0;
    m_BlendFunction.SourceConstantAlpha = 0xff;
    m_BlendFunction.AlphaFormat = AC_SRC_ALPHA;

    Resize(m_Size);
}

SrWindow::~SrWindow()
{
    const wchar_t* className = L"WINDOWCLASS";
    UnregisterClass(className, m_HInstance);
}

void SrWindow::Update()
{
    HDC DeviceContext = GetDC(m_HWnd);
    RECT rect = {};
    GetClientRect(m_HWnd, &rect);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;
    SetStretchBltMode(DeviceContext, BLACKONWHITE);
    StretchDIBits(
        DeviceContext,
        0, 0, windowWidth, windowHeight,
        0, 0, m_Size.x, m_Size.y,
        m_BitMapMemory,
        &m_BitMapInfo,
        DIB_RGB_COLORS, SRCCOPY
    );
    ReleaseDC(m_HWnd, DeviceContext);
}

void SrWindow::Resize(const glm::ivec2& size)
{
    if (m_BitMapMemory)
    {
        VirtualFree(m_BitMapMemory, 0, MEM_RELEASE);
    }

    m_Size = (glm::vec2)size / m_ScreenPixelsInBitMapPixels;

    m_BitMapInfo.bmiHeader.biSize = sizeof(m_BitMapInfo.bmiHeader);
    m_BitMapInfo.bmiHeader.biWidth = m_Size.x;
    m_BitMapInfo.bmiHeader.biHeight = m_Size.y;
    m_BitMapInfo.bmiHeader.biPlanes = 1;
    m_BitMapInfo.bmiHeader.biBitCount = 32;
    m_BitMapInfo.bmiHeader.biCompression = BI_RGB;
    m_BitMapInfo.bmiHeader.biSizeImage = m_Size.x * m_Size.y * channels;

    int m_BitMapMemorySize = m_Size.x * m_Size.y * channels;
    m_BitMapMemory = VirtualAlloc(0, m_BitMapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

void SrWindow::NewFrame()
{
    ProcessMessages();
}

void SrWindow::EndFrame()
{
}

void SrWindow::Clear(const glm::vec4& color)
{
    if (!m_BitMapMemory)
    {
        return;
    }

    for (size_t i = 0; i < m_Size.x; i++)
    {
        for (size_t j = 0; j < m_Size.y; j++)
        {
            int position = (i + j * m_Size.x) * channels;
            uint8_t* pixel = &((uint8_t*)m_BitMapMemory)[position];
            pixel[0] = (uint8_t)(color[2] * 255);
            pixel[1] = (uint8_t)(color[1] * 255);
            pixel[2] = (uint8_t)(color[0] * 255);
        }
    }
}

void SrWindow::Present(std::shared_ptr<Texture> texture)
{
    std::shared_ptr<SrTexture> srTexture = std::static_pointer_cast<SrTexture>(texture);

    uint8_t* textureData = srTexture->GetData();

    for (size_t i = 0; i < m_Size.x; i++)
    {
        for (size_t j = 0; j < m_Size.y; j++)
        {
            uint8_t* windowTexel = &((uint8_t*)m_BitMapMemory)[(i + j * m_Size.x) * channels];

            glm::vec2 uv = { (float)i / (float)m_Size.x, (float)j / (float)m_Size.y };
            glm::ivec4 texel = srTexture->Sample(uv);

            windowTexel[0] = texel[2];
            windowTexel[1] = texel[1];
            windowTexel[2] = texel[0];
        }
    }
}

void SrWindow::ShutDownPrepare()
{

}

LRESULT SrWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    std::shared_ptr<SrWindow> window = std::static_pointer_cast<SrWindow>(
        WindowManager::GetInstance().GetCurrentWindow());

    if (!window || window->m_HWnd != hWnd)
    {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    switch (uMsg)
    {
    case WM_CLOSE:
    {
        window->SetIsRunning(false);
        DestroyWindow(hWnd);
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    case WM_SIZE:
    {
        RECT rect;
        GetClientRect(hWnd, &rect);
        window->Resize({ rect.right - rect.left, rect.bottom - rect.top });
        break;
    }
    default:
    {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
        break;
    }
    }
}

void SrWindow::ProcessMessages()
{
    MSG msg{};
    while (PeekMessage(&msg, m_HWnd, 0u, 0u, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
