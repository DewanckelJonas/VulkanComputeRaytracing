#pragma once
#include "Platform.h"
namespace vkw
{
	class CommandPool;
	class VulkanDevice;
	class Texture
	{
	public:
		Texture(VulkanDevice* pDevice, CommandPool* cmdPool, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memPropFlags, VkImageLayout imageLayout, void* data, uint32_t width, uint32_t height, uint32_t layers = 1);
		~Texture();

		VkDescriptorImageInfo GetDescriptor();
		VkImage GetImage();
		VkImageLayout GetImageLayout();
		uint32_t GetWidth();
		uint32_t GetHeight();
		uint32_t GetLayers();
		void CopyTo(Texture* texture, VkCommandPool cmdPool, uint32_t sourceLayer = 0, uint32_t destLayer = 0);

	private:
		void Init(CommandPool* cmdPool, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memPropFlags, void* data);
		void Cleanup();

		void UpdateDescriptor();

		VulkanDevice*			m_pDevice;
		VkImage					m_Image;
		VkImageLayout			m_ImageLayout;
		VkDeviceMemory			m_DeviceMemory;
		VkImageView				m_ImageView;
		uint32_t				m_Width, m_Height, m_Layers;
		VkDescriptorImageInfo	m_Descriptor;
		VkSampler				m_Sampler;
	};
}

