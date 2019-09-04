#include "VulkanBaseApp.h"
#include "Window.h"
#include "VulkanSwapchain.h"
#include "VulkanDevice.h"
#include "VulkanHelpers.h"
#include "DepthStencilBuffer.h"
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "CommandPool.h"

using namespace vkw;

VulkanBaseApp::VulkanBaseApp(VulkanDevice * pDevice, const std::string & appName)
	:m_pDevice(pDevice), m_AppName(appName)
{
}

VulkanBaseApp::~VulkanBaseApp()
{
}

bool vkw::VulkanBaseApp::Update(float dTime)
{
	m_pWindow->Update();
	return m_IsRunning;
}

void VulkanBaseApp::Init(float width, float height)
{
	InitWindow(width, height);
	InitSwapchain();
	InitSynchronizations();
	InitDepthStencilBuffer();
	InitRenderPass();
	InitFramebuffers();
	InitCommandPool();
	AllocateDrawCommandBuffers();
	return;
}

void VulkanBaseApp::Close()
{
	m_IsRunning = false;
}

std::string VulkanBaseApp::GetName()
{
	return m_AppName;
}

void VulkanBaseApp::InitWindow(float width, float height)
{
	m_pWindow = new Window(this, m_pDevice, width, height);
}

void VulkanBaseApp::InitSynchronizations()
{
	//Semaphores
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	ErrorCheck(vkCreateSemaphore(m_pDevice->GetDevice(), &semaphoreCreateInfo, nullptr, &m_PresentCompleteSemaphore));

	ErrorCheck(vkCreateSemaphore(m_pDevice->GetDevice(), &semaphoreCreateInfo, nullptr, &m_RenderCompleteSemaphore));

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	m_WaitFences.resize(m_pSwapchain->GetImageCount());
	for (size_t i = 0; i < m_WaitFences.size(); i++)
	{
		ErrorCheck(vkCreateFence(m_pDevice->GetDevice(), &fenceCreateInfo, nullptr, &m_WaitFences[i]));
	}
}

void VulkanBaseApp::InitSwapchain(VkPresentModeKHR preferredPresentMode, uint32_t swapchainImageCount)
{
	m_pSwapchain = new VulkanSwapchain(m_pDevice, m_pWindow, preferredPresentMode, swapchainImageCount);
}

void VulkanBaseApp::InitDepthStencilBuffer()
{
	m_pDepthStencilBuffer = new DepthStencilBuffer(m_pDevice, m_pWindow);
}

void VulkanBaseApp::InitRenderPass()
{
	m_pRenderPass = new RenderPass(m_pDevice, m_pWindow, m_pDepthStencilBuffer);
}

void VulkanBaseApp::InitFramebuffers()
{
	m_FrameBuffers.resize(m_pSwapchain->GetImageCount());
	std::vector<VkImageView> swapchainImageViews = m_pSwapchain->GetImageViews();
	for (size_t i = 0; i < m_FrameBuffers.size(); i++)
	{
		std::vector<VkImageView> attachments{ m_pDepthStencilBuffer->GetImageView(), swapchainImageViews[i] };
		m_FrameBuffers[i] = new FrameBuffer(m_pDevice, m_pRenderPass, m_pWindow, attachments);
	}
}

void vkw::VulkanBaseApp::InitCommandPool(VkCommandPoolCreateFlags flags)
{
	m_pCommandPool = new CommandPool(m_pDevice, flags, m_pDevice->GetGraphicsFamilyQueueId());
}

void vkw::VulkanBaseApp::AllocateDrawCommandBuffers()
{
	m_DrawCommandBuffers.resize(m_pSwapchain->GetImageCount());

	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = m_pCommandPool->GetHandle();
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = m_DrawCommandBuffers.size();

	ErrorCheck(vkAllocateCommandBuffers(m_pDevice->GetDevice(), &commandBufferAllocateInfo, m_DrawCommandBuffers.data()));
}

void VulkanBaseApp::Cleanup()
{
	FreeDrawCommandBuffers();
	CleanupCommandPool();
	CleanupRenderPass();
	CleanupFramebuffers();
	CleanupDepthStencilBuffer();
	CleanupSynchronizations();
	CleanupSwapchain();
	CleanupWindow();
	return;
}

void VulkanBaseApp::CleanupWindow()
{
	delete m_pWindow;
}

void vkw::VulkanBaseApp::CleanupSynchronizations()
{
	vkDestroySemaphore(m_pDevice->GetDevice(), m_PresentCompleteSemaphore, nullptr);
	vkDestroySemaphore(m_pDevice->GetDevice(), m_RenderCompleteSemaphore, nullptr);
	for (size_t i = 0; i < m_WaitFences.size(); i++)
	{
		vkDestroyFence(m_pDevice->GetDevice(), m_WaitFences[i], nullptr);
	}
}

void VulkanBaseApp::CleanupSwapchain()
{
	delete m_pSwapchain;
}

void VulkanBaseApp::CleanupDepthStencilBuffer()
{
	delete m_pDepthStencilBuffer;
}

void VulkanBaseApp::CleanupRenderPass()
{
	delete m_pRenderPass;
}

void VulkanBaseApp::CleanupFramebuffers()
{
	for (size_t i = 0; i < m_FrameBuffers.size(); i++)
	{
		delete m_FrameBuffers[i];
	}
}

void vkw::VulkanBaseApp::CleanupCommandPool()
{
	delete m_pCommandPool;
}

void vkw::VulkanBaseApp::FreeDrawCommandBuffers()
{
	vkFreeCommandBuffers(m_pDevice->GetDevice(), m_pCommandPool->GetHandle(), m_DrawCommandBuffers.size(), m_DrawCommandBuffers.data());
}

bool vkw::VulkanBaseApp::IsRunning()
{
	return m_IsRunning;
}

VulkanDevice * VulkanBaseApp::GetDevice()
{
	return m_pDevice;
}

VulkanSwapchain * vkw::VulkanBaseApp::GetSwapchain()
{
	return m_pSwapchain;
}

Window * vkw::VulkanBaseApp::GetWindow()
{
	return m_pWindow;
}

RenderPass * vkw::VulkanBaseApp::GetRenderPass()
{
	return m_pRenderPass;
}

CommandPool * vkw::VulkanBaseApp::GetCommandPool()
{
	return m_pCommandPool;
}

const VkSemaphore& vkw::VulkanBaseApp::GetPresentCompleteSemaphore()
{
	return m_PresentCompleteSemaphore;
}

const VkSemaphore& vkw::VulkanBaseApp::GetRenderCompleteSemaphore()
{
	return m_RenderCompleteSemaphore;
}

const std::vector<VkFence>& vkw::VulkanBaseApp::GetWaitFences()
{
	return m_WaitFences;
}

const std::vector<VkCommandBuffer>& vkw::VulkanBaseApp::GetDrawCommandBuffers()
{
	return m_DrawCommandBuffers;
}

const std::vector<FrameBuffer*>& vkw::VulkanBaseApp::GetFrameBuffers()
{
	return m_FrameBuffers;
}

