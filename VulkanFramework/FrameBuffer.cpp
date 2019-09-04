#include "FrameBuffer.h"
#include "VulkanHelpers.h"
#include <vector>
#include "Window.h"
#include "RenderPass.h"
#include "VulkanDevice.h"

using namespace vkw;


vkw::FrameBuffer::FrameBuffer(VulkanDevice * pDevice, RenderPass * pRenderPass, Window * pWindow, const std::vector<VkImageView>& attachments)
	:m_pDevice(pDevice)
{
	Init(attachments, pRenderPass, pWindow);
}

FrameBuffer::~FrameBuffer()
{
	Cleanup();
}

VkFramebuffer vkw::FrameBuffer::GetHandle()
{
	return m_FrameBuffer;
}

void FrameBuffer::Init(const std::vector<VkImageView>& attachments, RenderPass* pRenderPass, Window* pWindow)
{

	VkFramebufferCreateInfo framebufferCreateInfo{};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.attachmentCount = attachments.size();
	framebufferCreateInfo.pAttachments = attachments.data();
	framebufferCreateInfo.renderPass = pRenderPass->GetHandle();
	framebufferCreateInfo.width = pWindow->GetSurfaceSize().width;
	framebufferCreateInfo.height = pWindow->GetSurfaceSize().height;
	framebufferCreateInfo.layers = 1;

	ErrorCheck(vkCreateFramebuffer(m_pDevice->GetDevice(), &framebufferCreateInfo, nullptr, &m_FrameBuffer));
}

void FrameBuffer::Cleanup()
{
	vkDestroyFramebuffer(m_pDevice->GetDevice(), m_FrameBuffer, nullptr);
}

