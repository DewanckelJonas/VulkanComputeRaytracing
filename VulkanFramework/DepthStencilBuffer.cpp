#include "DepthStencilBuffer.h"
#include <vector>
#include "VulkanDevice.h"
#include "Window.h"
#include "VulkanHelpers.h"

using namespace vkw;

DepthStencilBuffer::DepthStencilBuffer(VulkanDevice* pDevice, Window* pWindow)
	:m_pDevice(pDevice), m_pWindow(pWindow)
{
	Init();
}


DepthStencilBuffer::~DepthStencilBuffer()
{
	Cleanup();
}

VkImage DepthStencilBuffer::GetImage()
{
	return m_Image;
}

VkImageView DepthStencilBuffer::GetImageView()
{
	return m_ImageView;
}

VkFormat DepthStencilBuffer::GetFormat()
{
	return m_Format;
}

void DepthStencilBuffer::Init()
{
	std::vector<VkFormat> tryFormats{
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D16_UNORM
	};

	for (VkFormat format : tryFormats)
	{
		VkFormatProperties formatProperties{};
		vkGetPhysicalDeviceFormatProperties(m_pDevice->GetPhysicalDevice(), format, &formatProperties);
		if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			m_Format = format;
			break;
		}
	}

	if (m_Format == VK_FORMAT_UNDEFINED)
	{
		assert(0 && "No suitable depth stencil format found!");
		std::exit(-1);
	}

	if (m_Format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
		m_Format == VK_FORMAT_D24_UNORM_S8_UINT ||
		m_Format == VK_FORMAT_D16_UNORM_S8_UINT)
	{
		m_StencilAvailable = true;
	}


	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = m_Format;
	imageCreateInfo.extent.width = m_pWindow->GetSurfaceSize().width;
	imageCreateInfo.extent.height = m_pWindow->GetSurfaceSize().height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
	imageCreateInfo.pQueueFamilyIndices = nullptr;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	ErrorCheck(vkCreateImage(m_pDevice->GetDevice(), &imageCreateInfo, nullptr, &m_Image));

	VkMemoryRequirements imageMemoryRequirements{};
	vkGetImageMemoryRequirements(m_pDevice->GetDevice(), m_Image, &imageMemoryRequirements);

	uint32_t memoryIndex = FindMemoryTypeIndex(&m_pDevice->GetPhysicalDeviceMemoryProperties(), &imageMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = imageMemoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex =

	vkAllocateMemory(m_pDevice->GetDevice(), &memoryAllocateInfo, nullptr, &m_ImageMemory);
	vkBindImageMemory(m_pDevice->GetDevice(), m_Image, m_ImageMemory, 0);

	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = m_Image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = m_Format;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // swizzle identity does 1 to 1 mapping the in this case the same as swizzle r
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (m_StencilAvailable ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	ErrorCheck(vkCreateImageView(m_pDevice->GetDevice(), &imageViewCreateInfo, nullptr, &m_ImageView));
}

void DepthStencilBuffer::Cleanup()
{
	vkDestroyImageView(m_pDevice->GetDevice(), m_ImageView, nullptr);
	vkFreeMemory(m_pDevice->GetDevice(), m_ImageMemory, nullptr);
	vkDestroyImage(m_pDevice->GetDevice(), m_Image, nullptr);
}
