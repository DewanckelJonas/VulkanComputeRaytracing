#pragma once
#include "Platform.h"
namespace vkw
{
	class VulkanDevice;
	class Window;
	class DepthStencilBuffer;
	class RenderPass
	{
	public:
		RenderPass(VulkanDevice* pDevice, Window* pWindow, DepthStencilBuffer* pDepthStencilBuffer);
		~RenderPass();

		VkRenderPass GetHandle();

	private:
		void Init();
		void Cleanup();

		VulkanDevice*				m_pDevice = nullptr;
		Window*						m_pWindow = nullptr;
		DepthStencilBuffer*			m_pDepthStencilBuffer = nullptr;

		VkRenderPass				m_RenderPass = VK_NULL_HANDLE;
	};
}

