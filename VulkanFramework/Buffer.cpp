#include "Buffer.h"
#include "VulkanHelpers.h"
#include "VulkanDevice.h"
#include "CommandPool.h"
#include <iostream>
using namespace vkw;

Buffer::Buffer(VulkanDevice * pDevice, CommandPool* cmdPool, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memPropFlags, size_t size, void * data)
	:m_pDevice(pDevice)
{
	Init(usageFlags, memPropFlags, size, data, cmdPool);
}

Buffer::~Buffer()
{
	Cleanup();
}

void vkw::Buffer::Update(void * data, size_t size, CommandPool* cmdPool)
{
	if(!m_UsingStagingBuffer)
	{
		void* pMappedMemory{};
		ErrorCheck(vkMapMemory(m_pDevice->GetDevice(), m_Memory, 0, VK_WHOLE_SIZE, 0, &pMappedMemory));
		memcpy(pMappedMemory, data, size);
		vkUnmapMemory(m_pDevice->GetDevice(), m_Memory);
		return;
	}
	std::cout << "Warning: Updating a non host visible and host coherent buffer requires a staging buffer which is more performance intensive! Consider using a host visible and host coherent buffer instead!" << std::endl;

	VkBuffer stagingBuffer{};
	VkDeviceMemory stagingBufferMemory{};
	CreateBuffer(
		m_pDevice->GetDevice(), m_pDevice->GetPhysicalDeviceMemoryProperties(),
		size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory
	);


	void* pMappedMemory{};
	ErrorCheck(vkMapMemory(m_pDevice->GetDevice(), stagingBufferMemory, 0, VK_WHOLE_SIZE, 0, &pMappedMemory));
	memcpy(pMappedMemory, data, size);
	vkUnmapMemory(m_pDevice->GetDevice(), stagingBufferMemory);
	
	CopyBuffer(m_pDevice->GetDevice(), m_pDevice->GetQueue(), cmdPool->GetHandle(), stagingBuffer, m_Buffer, size);

	vkFreeMemory(m_pDevice->GetDevice(), stagingBufferMemory, nullptr);
	vkDestroyBuffer(m_pDevice->GetDevice(), stagingBuffer, nullptr);

}

VkDescriptorBufferInfo vkw::Buffer::GetDescriptor()
{
	return m_Descriptor;
}

void Buffer::Init(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memPropFlags, size_t size, void* data, CommandPool* cmdPool)
{
	m_Size = size;
	m_UsingStagingBuffer = (memPropFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) != (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VkBuffer buffer{};
	VkDeviceMemory bufferMemory{};
	if (m_UsingStagingBuffer && data != nullptr) 
	{
		CreateBuffer(
			m_pDevice->GetDevice(), m_pDevice->GetPhysicalDeviceMemoryProperties(),
			size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			buffer, bufferMemory
		);
	}else
	{
		CreateBuffer(
			m_pDevice->GetDevice(), m_pDevice->GetPhysicalDeviceMemoryProperties(),
			size, usageFlags, memPropFlags,
			buffer, bufferMemory
		);
	}
	if (data != nullptr)
	{
		void* pMappedMemory{};
		ErrorCheck(vkMapMemory(m_pDevice->GetDevice(),bufferMemory, 0, VK_WHOLE_SIZE, 0, &pMappedMemory));
		memcpy(pMappedMemory, data, size);
		vkUnmapMemory(m_pDevice->GetDevice(), bufferMemory);
	}

	if (!m_UsingStagingBuffer || data == nullptr) {
		m_Buffer = buffer;
		m_Memory = bufferMemory;
		UpdateDescriptor();
		return;
	}

	CreateBuffer(
		m_pDevice->GetDevice(), m_pDevice->GetPhysicalDeviceMemoryProperties(),
		size, usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, memPropFlags,
		m_Buffer, m_Memory
	);

	CopyBuffer(m_pDevice->GetDevice(), m_pDevice->GetQueue(), cmdPool->GetHandle(), buffer, m_Buffer, size);

	vkFreeMemory(m_pDevice->GetDevice(), bufferMemory, nullptr);
	vkDestroyBuffer(m_pDevice->GetDevice(), buffer, nullptr);

	UpdateDescriptor();
}

void vkw::Buffer::Cleanup()
{
	vkFreeMemory(m_pDevice->GetDevice(), m_Memory, nullptr);
	vkDestroyBuffer(m_pDevice->GetDevice(), m_Buffer, nullptr);
}

void vkw::Buffer::UpdateDescriptor()
{
	m_Descriptor.buffer = m_Buffer;
	m_Descriptor.offset = 0;
	m_Descriptor.range = m_Size;
}
