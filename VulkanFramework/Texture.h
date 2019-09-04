#pragma once
#include "Platform.h"
namespace vkw
{
	class CommandPool;
	class VulkanDevice;
	class Texture
	{
	public:
		Texture(VulkanDevice* pDevice, CommandPool* cmdPool, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memPropFlags, VkImageLayout imageLayout, void* data, uint32_t width, uint32_t height);
		~Texture();

		VkDescriptorImageInfo GetDescriptor();
		VkImage GetImage();
		uint32_t GetWidth();
		uint32_t GetHeight();

	private:
		void Init(CommandPool* cmdPool, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memPropFlags, void* data = nullptr);
		void Cleanup();

		void UpdateDescriptor();

		VulkanDevice*			m_pDevice;
		VkImage					m_Image;
		VkImageLayout			m_ImageLayout;
		VkDeviceMemory			m_DeviceMemory;
		VkImageView				m_ImageView;
		uint32_t				m_Width, m_Height;
		VkDescriptorImageInfo	m_Descriptor;
		VkSampler				m_Sampler;
	};
}

