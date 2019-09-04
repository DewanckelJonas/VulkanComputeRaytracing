#pragma once
#include "Platform.h"
#include <vector>
namespace vkw
{
	class VulkanDevice;
	class CommandPool
	{
	public:
		CommandPool(VulkanDevice* pDevice, VkCommandPoolCreateFlags flags, uint32_t familyQueueId);
		~CommandPool();

		std::vector<VkCommandBuffer> CreateCommandBuffers(size_t amount, VkCommandBufferLevel bufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VkCommandPool GetHandle();
	private:
		void Init();
		void Cleanup();

		VkCommandPool				m_CommandPool = VK_NULL_HANDLE;
		VulkanDevice*				m_pDevice = nullptr;
		VkCommandPoolCreateFlags	m_Flags{};
		uint32_t					m_FamilyQueueId{};
	};
}

