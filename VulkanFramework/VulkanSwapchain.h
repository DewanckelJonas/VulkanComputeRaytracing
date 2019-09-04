#pragma once
#include "Platform.h"
#include <vector>
namespace vkw 
{
	class VulkanDevice;
	class Window;
	class VulkanSwapchain
	{
	public:
		VulkanSwapchain(VulkanDevice* pDevice, Window* pWindow, VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR, uint32_t swapchainImageCount = 2);
		~VulkanSwapchain();

		uint32_t AcquireNextImage(VkSemaphore presentComplete);
		void PresentImage(VkSemaphore waitSemaphore);
		uint32_t GetActiveImageId();
		std::vector<VkImage> GetImages();
		std::vector<VkImageView> GetImageViews();
		size_t GetImageCount();

	private:
		void Init();
		void InitImages();

		void Cleanup();
		void CleanupImages();

		VulkanDevice*				m_pDevice = nullptr;
		Window*						m_pWindow = nullptr;

		uint32_t					m_SwapchainImageCount{};
		VkSwapchainKHR				m_Swapchain = VK_NULL_HANDLE;
		std::vector<VkImage>		m_SwapchainImages{};
		std::vector<VkImageView>	m_SwapchainImageViews{};
		const VkPresentModeKHR		m_PreferredPresentMode{};

		uint32_t					m_ActiveSwapchainImageId{};

	};
}

