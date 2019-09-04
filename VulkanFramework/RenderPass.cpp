#include "RenderPass.h"
#include "VulkanDevice.h"
#include "DepthStencilBuffer.h"
#include "Window.h"
#include "VulkanHelpers.h"
#include <array>

using namespace vkw;

RenderPass::RenderPass(VulkanDevice* pDevice, Window* pWindow, DepthStencilBuffer* pDepthStencilBuffer)
	:m_pDevice(pDevice), m_pWindow(pWindow), m_pDepthStencilBuffer(pDepthStencilBuffer)
{
	Init();
}


RenderPass::~RenderPass()
{
	Cleanup();
}

VkRenderPass vkw::RenderPass::GetHandle()
{
	return m_RenderPass;
}

void RenderPass::Init()
{
	std::vector<VkAttachmentDescription>attachments{1};
	int idx = 0;
	if (m_pDepthStencilBuffer != nullptr)
	{
		attachments.resize(2);
		//depth stencil attachment
		attachments[idx].flags = 0;
		attachments[idx].format =	m_pDepthStencilBuffer->GetFormat();
		attachments[idx].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[idx].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[idx].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[idx].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[idx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[idx].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[idx].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		++idx;
	}

	//surface attachment
	attachments[idx].flags = 0;
	attachments[idx].format = m_pWindow->GetSurfaceFormat().format;
	attachments[idx].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[idx].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[idx].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[idx].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[idx].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference subPass0DepthStencilAttachment{};
	subPass0DepthStencilAttachment.attachment = 0;
	subPass0DepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentReference, 1> subPass0ColorAttachments{};
	subPass0ColorAttachments[0].attachment = 1; //this int is the index of the attachments passed to the renderPass.
	subPass0ColorAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array<VkSubpassDescription, 1> subPasses{};
	subPasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	//subPasses[0].inputAttachmentCount = 0;
	//subPasses[0].pInputAttachments = nullptr;
	subPasses[0].colorAttachmentCount = subPass0ColorAttachments.size();
	subPasses[0].pColorAttachments = subPass0ColorAttachments.data();
	subPasses[0].pDepthStencilAttachment = &subPass0DepthStencilAttachment;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = attachments.size();
	renderPassCreateInfo.pAttachments = attachments.data();
	renderPassCreateInfo.subpassCount = subPasses.size();
	renderPassCreateInfo.pSubpasses = subPasses.data();
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	ErrorCheck(vkCreateRenderPass(m_pDevice->GetDevice(), &renderPassCreateInfo, nullptr, &m_RenderPass));
}

void vkw::RenderPass::Cleanup()
{
	vkDestroyRenderPass(m_pDevice->GetDevice(), m_RenderPass, nullptr);
}
