#include "Texture.h"
#include "VulkanDevice.h"
#include "VulkanHelpers.h"
#include "CommandPool.h"
using namespace vkw;

Texture::Texture(VulkanDevice* pDevice, CommandPool* cmdPool, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memPropFlags, VkImageLayout imageLayout, void* data, uint32_t width, uint32_t height)
	:m_ImageLayout(imageLayout), m_pDevice(pDevice), m_Width(width), m_Height(height)
{
	Init(cmdPool, usageFlags, memPropFlags, data);
}


Texture::~Texture()
{
	Cleanup();
}

VkDescriptorImageInfo vkw::Texture::GetDescriptor()
{
	return m_Descriptor;
}

VkImage vkw::Texture::GetImage()
{
	return m_Image;
}

uint32_t vkw::Texture::GetWidth()
{
	return m_Width;
}

uint32_t vkw::Texture::GetHeight()
{
	return m_Height;
}


void vkw::Texture::Init(CommandPool* cmdPool, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memPropFlags, void* data)
{
	CreateImage(m_pDevice->GetDevice(), m_pDevice->GetPhysicalDeviceMemoryProperties(), m_Width, m_Height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, usageFlags, memPropFlags, m_Image, m_DeviceMemory);



	if(data != nullptr)
	{
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		VkDeviceSize imageSize = m_Width * m_Height * 4;

		CreateBuffer(m_pDevice->GetDevice(), m_pDevice->GetPhysicalDeviceMemoryProperties(), imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* mappedMemory;
		vkMapMemory(m_pDevice->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &mappedMemory);
		memcpy(mappedMemory, data, static_cast<size_t>(imageSize));
		vkUnmapMemory(m_pDevice->GetDevice(), stagingBufferMemory);

		TransitionImageLayout(m_pDevice->GetDevice(), m_pDevice->GetQueue(), cmdPool->GetHandle(), m_Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(m_pDevice->GetDevice(), m_pDevice->GetQueue(), cmdPool->GetHandle(), stagingBuffer, m_Image, m_Width, m_Height);

		vkDestroyBuffer(m_pDevice->GetDevice(), stagingBuffer, nullptr);
		vkFreeMemory(m_pDevice->GetDevice(), stagingBufferMemory, nullptr);

		TransitionImageLayout(m_pDevice->GetDevice(), m_pDevice->GetQueue(), cmdPool->GetHandle(), m_Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_ImageLayout);
		return;
	}

	TransitionImageLayout(m_pDevice->GetDevice(), m_pDevice->GetQueue(), cmdPool->GetHandle(), m_Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, m_ImageLayout);

	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	ErrorCheck(vkCreateSampler(m_pDevice->GetDevice(), &samplerCreateInfo, nullptr, &m_Sampler));

	
	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	imageViewCreateInfo.image = m_Image;
	ErrorCheck(vkCreateImageView(m_pDevice->GetDevice(), &imageViewCreateInfo, nullptr, &m_ImageView));

	UpdateDescriptor();

}

void vkw::Texture::Cleanup()
{
	vkDestroySampler(m_pDevice->GetDevice(), m_Sampler, nullptr);
	vkDestroyImageView(m_pDevice->GetDevice(), m_ImageView, nullptr);
	vkDestroyImage(m_pDevice->GetDevice(), m_Image, nullptr);
	vkFreeMemory(m_pDevice->GetDevice(), m_DeviceMemory, nullptr);
}

void Texture::UpdateDescriptor()
{
	m_Descriptor.sampler = m_Sampler;
	m_Descriptor.imageView = m_ImageView;
	m_Descriptor.imageLayout = m_ImageLayout;
}