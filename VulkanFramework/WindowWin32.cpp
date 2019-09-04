#include "BUILD_OPTIONS.h"
#include "Platform.h"
#include "Window.h"
#include <assert.h>
#include "VulkanDevice.h"
#include "VulkanBaseApp.h"

using namespace vkw;

#if VK_USE_PLATFORM_WIN32_KHR

LRESULT CALLBACK WindowsEventHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	VulkanBaseApp * pApp = reinterpret_cast<VulkanBaseApp*>(
		GetWindowLongPtrW(hWnd, GWLP_USERDATA));

	switch (uMsg) {
	case WM_CLOSE:
		pApp->Close();
		return 0;
	case WM_SIZE:
		// we get here if the window has changed size, we should rebuild most
		// of our window resources before rendering to this window again.
		// ( no need for this because our window sizing by hand is disabled )
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

uint64_t	Window::m_Win32ClassIdCounter = 0;

void Window::Init()
{
	WNDCLASSEX winClass{};
	assert(m_SurfaceSize.width > 0);
	assert(m_SurfaceSize.height > 0);

	m_Win32Instance = GetModuleHandle(nullptr);
	m_Win32ClassName = m_pApp->GetName() + "_" + std::to_string(m_Win32ClassIdCounter);
	m_Win32ClassIdCounter++;

	// Initialize the window class structure:
	winClass.cbSize = sizeof(WNDCLASSEX);
	winClass.style = CS_HREDRAW | CS_VREDRAW;
	winClass.lpfnWndProc = WindowsEventHandler;
	winClass.cbClsExtra = 0;
	winClass.cbWndExtra = 0;
	winClass.hInstance = m_Win32Instance; // hInstance
	winClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	winClass.lpszMenuName = NULL;
	winClass.lpszClassName = m_Win32ClassName.c_str();
	winClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	// Register window class:
	if (!RegisterClassEx(&winClass)) {
		// It didn't work, so try to give a useful error:
		assert(0 && "Cannot create a window in which to draw!\n");
		fflush(stdout);
		std::exit(-1);
	}

	DWORD ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	// Create window with the registered class:
	RECT wr = { 0, 0, LONG(m_SurfaceSize.width), LONG(m_SurfaceSize.height) };
	AdjustWindowRectEx(&wr, style, FALSE, ex_style);
	m_Win32Window = CreateWindowEx(0,
		m_Win32ClassName.c_str(),		// class name
		m_pApp->GetName().c_str(),			// app name
		style,							// window style
		CW_USEDEFAULT, CW_USEDEFAULT,	// x/y coords
		wr.right - wr.left,				// width
		wr.bottom - wr.top,				// height
		NULL,							// handle to parent
		NULL,							// handle to menu
		m_Win32Instance,				// hInstance
		NULL);							// no extra parameters
	if (!m_Win32Window) {
		// It didn't work, so try to give a useful error:
		assert(1 && "Cannot create a window in which to draw!\n");
		fflush(stdout);
		std::exit(-1);
	}
	SetWindowLongPtr(m_Win32Window, GWLP_USERDATA, (LONG_PTR)m_pApp);

	ShowWindow(m_Win32Window, SW_SHOW);
	SetForegroundWindow(m_Win32Window);
	SetFocus(m_Win32Window);

	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = m_Win32Instance;
	surfaceCreateInfo.hwnd = m_Win32Window;
	vkCreateWin32SurfaceKHR(m_pDevice->GetInstance(), &surfaceCreateInfo, nullptr, &m_Surface);
}


void Window::Destroy()
{
	DestroyWindow(m_Win32Window);
	UnregisterClass(m_Win32ClassName.c_str(), m_Win32Instance);
}

void Window::Update()
{
	MSG msg;
	if (PeekMessage(&msg, m_Win32Window, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

#endif