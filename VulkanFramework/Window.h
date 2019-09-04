#pragma once
#include "Platform.h"
#include <string>

namespace vkw 
{
	class VulkanDevice;
	class VulkanBaseApp;
	class Window
	{
	public:
		Window(VulkanBaseApp* app, VulkanDevice* device, uint32_t width, uint32_t height);
		~Window();
		void Update();

		VkSurfaceKHR GetSurface();
		VkExtent2D GetSurfaceSize();
		VkSurfaceCapabilitiesKHR GetSurfaceCapabilities();
		VkSurfaceFormatKHR GetSurfaceFormat();

	private:

		void Init();
		void InitSurface();
		void CleanupSurface();
		void Destroy();

		VulkanBaseApp*						m_pApp = nullptr;
		VulkanDevice*						m_pDevice = nullptr;

		VkSurfaceKHR						m_Surface = VK_NULL_HANDLE;
		VkExtent2D							m_SurfaceSize{};
		VkSurfaceCapabilitiesKHR			m_SurfaceCapabilities{};
		VkSurfaceFormatKHR					m_SurfaceFormat{};

		//Platfrom specific members

#if VK_USE_PLATFORM_WIN32_KHR
		HINSTANCE							m_Win32Instance = NULL;
		HWND								m_Win32Window = NULL;
		std::string							m_Win32ClassName{};
		static uint64_t						m_Win32ClassIdCounter;
#endif

	};
}

