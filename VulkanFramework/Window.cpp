#include "Window.h"
#include "VulkanDevice.h"
#include "VulkanHelpers.h"

using namespace vkw;

	Window::Window(VulkanBaseApp * app, VulkanDevice * device, uint32_t width, uint32_t height)
		:m_pApp(app), m_pDevice(device)
		, m_SurfaceSize{ width, height }
	{
		Init();
		InitSurface();
	}

	Window::~Window()
	{
		CleanupSurface();
		Destroy();
	}

	VkSurfaceKHR vkw::Window::GetSurface()
	{
		return m_Surface;
	}

	VkExtent2D vkw::Window::GetSurfaceSize()
	{
		return m_SurfaceSize;
	}

	VkSurfaceCapabilitiesKHR vkw::Window::GetSurfaceCapabilities()
	{
		return m_SurfaceCapabilities;
	}

	VkSurfaceFormatKHR vkw::Window::GetSurfaceFormat()
	{
		return m_SurfaceFormat;
	}

	void vkw::Window::InitSurface()
	{
		VkPhysicalDevice gpu = m_pDevice->GetPhysicalDevice();

		VkBool32 WSISupported = false;
		ErrorCheck(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, m_pDevice->GetGraphicsFamilyQueueId(), m_Surface, &WSISupported));
		if (!WSISupported)
		{
			assert(0 && "WSI not supported");
			std::exit(-1);
		}

		ErrorCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, m_Surface, &m_SurfaceCapabilities));
		if (m_SurfaceCapabilities.currentExtent.width < UINT32_MAX)
		{
			m_SurfaceSize.width = m_SurfaceCapabilities.currentExtent.width;
			m_SurfaceSize.height = m_SurfaceCapabilities.currentExtent.height;
		}

		{
			uint32_t formatCount = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_Surface, &formatCount, nullptr);
			if (formatCount == 0) {
				assert(0 && "No surface formats found");
				std::exit(-1);
			}
			std::vector<VkSurfaceFormatKHR> formats{ formatCount };
			vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_Surface, &formatCount, formats.data());

			if (formats[0].format == VK_FORMAT_UNDEFINED) {
				m_SurfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
				m_SurfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
			}
			else
			{
				m_SurfaceFormat = formats[0];
			}
		}

	}

	void vkw::Window::CleanupSurface()
	{
		vkDestroySurfaceKHR(m_pDevice->GetInstance(), m_Surface, nullptr);
	}
