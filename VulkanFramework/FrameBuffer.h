#pragma once
#include "Platform.h"
#include <vector>
namespace vkw
{
	class VulkanDevice;
	class RenderPass;
	class Window;
	class FrameBuffer
	{
	public:
		FrameBuffer(VulkanDevice * pDevice, RenderPass * pRenderPass, Window * pWindow, const std::vector<VkImageView>& attachments);
		~FrameBuffer();
		VkFramebuffer GetHandle();

	private:
		void Init(const std::vector<VkImageView>& attachments, RenderPass* pRenderPass, Window* pWindow);
		void Cleanup();

		VulkanDevice*		m_pDevice = nullptr;
		VkFramebuffer		m_FrameBuffer = VK_NULL_HANDLE;
	};
}

