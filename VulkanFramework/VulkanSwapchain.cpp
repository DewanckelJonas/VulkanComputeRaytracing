#include "VulkanSwapchain.h"
#include "VulkanHelpers.h"
#include "VulkanDevice.h"
#include "Window.h"
using namespace vkw;

VulkanSwapchain::VulkanSwapchain(VulkanDevice* pDevice, Window* pWindow, VkPresentModeKHR preferredPresentMode, uint32_t swapchainImageCount)
	:m_pDevice(pDevice)
	,m_pWindow(pWindow)
	,m_PreferredPresentMode(preferredPresentMode)
	,m_SwapchainImageCount(swapchainImageCount)
{
	Init();
	InitImages();
}


VulkanSwapchain::~VulkanSwapchain()
{
	CleanupImages();
	Cleanup();
}

uint32_t vkw::VulkanSwapchain::AcquireNextImage(VkSemaphore presentComplete)
{
	vkAcquireNextImageKHR(m_pDevice->GetDevice(), m_Swapchain, UINT64_MAX, presentComplete, nullptr, &m_ActiveSwapchainImageId);
	return m_ActiveSwapchainImageId;
}

void vkw::VulkanSwapchain::PresentImage(VkSemaphore waitSemaphore)
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.pImageIndices = &m_ActiveSwapchainImageId;
	
	if (waitSemaphore != VK_NULL_HANDLE){
		presentInfo.pWaitSemaphores = &waitSemaphore;
		presentInfo.waitSemaphoreCount = 1;
	}
	ErrorCheck(vkQueuePresentKHR(m_pDevice->GetQueue(), &presentInfo));
	return;
}

uint32_t vkw::VulkanSwapchain::GetActiveImageId()
{
	return m_ActiveSwapchainImageId;
}

std::vector<VkImage> vkw::VulkanSwapchain::GetImages()
{
	return m_SwapchainImages;
}

std::vector<VkImageView> vkw::VulkanSwapchain::GetImageViews()
{
	return m_SwapchainImageViews;
}

size_t vkw::VulkanSwapchain::GetImageCount()
{
	return m_SwapchainImages.size();
}

void VulkanSwapchain::Init()
{

	VkSurfaceKHR surface = m_pWindow->GetSurface();
	VkSurfaceCapabilitiesKHR surfaceCapabilities = m_pWindow->GetSurfaceCapabilities();

	if (m_SwapchainImageCount < surfaceCapabilities.minImageCount + 1) m_SwapchainImageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0) {
		if (m_SwapchainImageCount > surfaceCapabilities.maxImageCount) m_SwapchainImageCount = surfaceCapabilities.maxImageCount;
	}

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	{
		uint32_t presentModeCount = 0;
		ErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(m_pDevice->GetPhysicalDevice(), surface, &presentModeCount, nullptr));
		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		ErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(m_pDevice->GetPhysicalDevice(), surface, &presentModeCount, presentModes.data()));

		for (const VkPresentModeKHR& mode : presentModes)
		{
			if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) presentMode = mode;
		}
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = m_SwapchainImageCount;
	swapchainCreateInfo.imageFormat = m_pWindow->GetSurfaceFormat().format;
	swapchainCreateInfo.imageColorSpace = m_pWindow->GetSurfaceFormat().colorSpace;
	swapchainCreateInfo.imageExtent = m_pWindow->GetSurfaceSize();
	swapchainCreateInfo.imageArrayLayers = 1; // can be used for stereoscopic rendering
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = 0;
	swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;


	ErrorCheck(vkCreateSwapchainKHR(m_pDevice->GetDevice(), &swapchainCreateInfo, nullptr, &m_Swapchain));

	ErrorCheck(vkGetSwapchainImagesKHR(m_pDevice->GetDevice(), m_Swapchain, &m_SwapchainImageCount, nullptr));
}

void VulkanSwapchain::InitImages()
{
	m_SwapchainImages.resize(m_SwapchainImageCount);
	m_SwapchainImageViews.resize(m_SwapchainImageCount);

	ErrorCheck(vkGetSwapchainImagesKHR(m_pDevice->GetDevice(), m_Swapchain, &m_SwapchainImageCount, m_SwapchainImages.data()));

	for (uint32_t i = 0; i < m_SwapchainImageCount; i++)
	{
		m_SwapchainImageViews[i] = CreateImageView(m_pDevice->GetDevice(), m_SwapchainImages[i], m_pWindow->GetSurfaceFormat().format);
	}
}

void VulkanSwapchain::Cleanup()
{
	vkDestroySwapchainKHR(m_pDevice->GetDevice(), m_Swapchain, nullptr);
}

void VulkanSwapchain::CleanupImages()
{
	for (VkImageView view : m_SwapchainImageViews)
	{
		vkDestroyImageView(m_pDevice->GetDevice(), view, nullptr);
	}
}
