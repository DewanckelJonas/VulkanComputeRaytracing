#pragma once
#include "Platform.h"

namespace vkw
{
	class VulkanDevice;
	class Window;
	class DepthStencilBuffer
	{
	public:
		DepthStencilBuffer(VulkanDevice* pDevice, Window* pWindow);
		~DepthStencilBuffer();

		VkImage GetImage();
		VkImageView GetImageView();
		VkFormat GetFormat();

	private:
		void Init();
		void Cleanup();

		VulkanDevice*		m_pDevice = nullptr;
		Window*				m_pWindow = nullptr;

		VkFormat			m_Format{};
		bool				m_StencilAvailable{false};
		VkImage				m_Image = VK_NULL_HANDLE;
		VkImageView			m_ImageView = VK_NULL_HANDLE;
		VkDeviceMemory		m_ImageMemory = VK_NULL_HANDLE;
	};
}

