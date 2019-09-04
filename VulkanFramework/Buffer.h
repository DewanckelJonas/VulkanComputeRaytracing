#pragma once
#include "Platform.h"

namespace vkw
{
	class VulkanDevice;
	class CommandPool;
	class Buffer
	{
	public:
		Buffer(VulkanDevice* pDevice, CommandPool* cmdPool, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memPropFlags, size_t size, void* data);
		~Buffer();

		void Update(void * data, size_t size, CommandPool* cmdPool);
		VkDescriptorBufferInfo GetDescriptor();

	private:
		void Init(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memPropFlags, size_t size, void* data, CommandPool* cmdPool);
		void Cleanup();
		void UpdateDescriptor();

		VulkanDevice*						m_pDevice = nullptr;
		VkBuffer							m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory						m_Memory = VK_NULL_HANDLE;
		VkDescriptorBufferInfo				m_Descriptor{};
		VkDeviceSize						m_Size{};
		bool								m_UsingStagingBuffer{ false };
	};
}

