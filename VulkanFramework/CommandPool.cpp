#include "CommandPool.h"
#include "VulkanDevice.h"
#include "VulkanHelpers.h"

using namespace vkw;



CommandPool::CommandPool(VulkanDevice* pDevice, VkCommandPoolCreateFlags flags, uint32_t familyQueueId)
	:m_pDevice(pDevice), m_Flags(flags), m_FamilyQueueId(familyQueueId)
{
	Init();
}

CommandPool::~CommandPool()
{
	Cleanup();
}

std::vector<VkCommandBuffer> CommandPool::CreateCommandBuffers(size_t amount, VkCommandBufferLevel bufferLevel)
{
	std::vector<VkCommandBuffer> commandBuffers{amount};

	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = m_CommandPool;
	commandBufferAllocateInfo.level = bufferLevel;
	commandBufferAllocateInfo.commandBufferCount = amount;

	ErrorCheck(vkAllocateCommandBuffers(m_pDevice->GetDevice(), &commandBufferAllocateInfo, commandBuffers.data()));

	return commandBuffers;
}

VkCommandPool vkw::CommandPool::GetHandle()
{
	return m_CommandPool;
}

void CommandPool::Init()
{
	VkCommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.flags = m_Flags;
	poolCreateInfo.queueFamilyIndex = m_FamilyQueueId;
	vkCreateCommandPool(m_pDevice->GetDevice(), &poolCreateInfo, nullptr, &m_CommandPool);
}

void vkw::CommandPool::Cleanup()
{
	vkDestroyCommandPool(m_pDevice->GetDevice(), m_CommandPool, nullptr);
}

