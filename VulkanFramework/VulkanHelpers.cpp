#include "Platform.h"
#include <iostream>
#include <assert.h>
#include "BUILD_OPTIONS.h"
#include "VulkanHelpers.h"
#include <array>

#if BUILD_ENABLE_VULKAN_RUNTIME_DEBUG
void ErrorCheck(VkResult result)
{
	if (result < 0)
	{
		switch (result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				std::cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
				break;
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				std::cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
				break;
			case VK_ERROR_INITIALIZATION_FAILED:
				std::cout << "VK_ERROR_INITIALIZATION_FAILED" << std::endl;
				break;
			case VK_ERROR_DEVICE_LOST:
				std::cout << "VK_ERROR_DEVICE_LOST" << std::endl;
				break;
			case VK_ERROR_MEMORY_MAP_FAILED:
				std::cout << "VK_ERROR_MEMORY_MAP_FAILED" << std::endl;
				break;
			case VK_ERROR_LAYER_NOT_PRESENT:
				std::cout << "VK_ERROR_LAYER_NOT_PRESENT" << std::endl;
				break;
			case VK_ERROR_EXTENSION_NOT_PRESENT:
				std::cout << "VK_ERROR_EXTENSION_NOT_PRESENT" << std::endl;
				break;
			case VK_ERROR_FEATURE_NOT_PRESENT:
				std::cout << "VK_ERROR_FEATURE_NOT_PRESENT" << std::endl;
				break;
			case VK_ERROR_INCOMPATIBLE_DRIVER:
				std::cout << "VK_ERROR_INCOMPATIBLE_DRIVER" << std::endl;
				break;
			case VK_ERROR_TOO_MANY_OBJECTS:
				std::cout << "VK_ERROR_TOO_MANY_OBJECTS" << std::endl;
				break;
			case VK_ERROR_FORMAT_NOT_SUPPORTED:
				std::cout << "VK_ERROR_FORMAT_NOT_SUPPORTED" << std::endl;
				break;
			case VK_ERROR_FRAGMENTED_POOL:
				std::cout << "VK_ERROR_FRAGMENTED_POOL" << std::endl;
				break;
			case VK_ERROR_OUT_OF_POOL_MEMORY:
				std::cout << "VK_ERROR_OUT_OF_POOL_MEMORY" << std::endl;
				break;
			case VK_ERROR_INVALID_EXTERNAL_HANDLE:
				std::cout << "VK_ERROR_INVALID_EXTERNAL_HANDLE" << std::endl;
				break;
			case VK_ERROR_SURFACE_LOST_KHR:
				std::cout << "VK_ERROR_SURFACE_LOST_KHR" << std::endl;
				break;
			case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
				std::cout << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" << std::endl;
				break;
			case VK_SUBOPTIMAL_KHR:
				std::cout << "VK_SUBOPTIMAL_KHR" << std::endl;
				break;
			case VK_ERROR_OUT_OF_DATE_KHR:
				std::cout << "VK_ERROR_OUT_OF_DATE_KHR" << std::endl;
				break;
			case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
				std::cout << "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" << std::endl;
				break;
			case VK_ERROR_VALIDATION_FAILED_EXT:
				std::cout << "VK_ERROR_VALIDATION_FAILED_EXT" << std::endl;
				break;
			case VK_ERROR_INVALID_SHADER_NV:
				std::cout << "VK_ERROR_INVALID_SHADER_NV" << std::endl;
				break;
			case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
				std::cout << "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT" << std::endl;
				break;
			case VK_ERROR_FRAGMENTATION_EXT:
				std::cout << "VK_ERROR_FRAGMENTATION_EXT" << std::endl;
				break;
			case VK_ERROR_NOT_PERMITTED_EXT:
				std::cout << "VK_ERROR_NOT_PERMITTED_EXT" << std::endl;
				break;
			case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
				std::cout << "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT" << std::endl;
				break;
			case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
				std::cout << "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" << std::endl;
				break;
			case VK_RESULT_RANGE_SIZE:
				std::cout << "VK_RESULT_RANGE_SIZE" << std::endl;
				break;
			case VK_RESULT_MAX_ENUM:
				std::cout << "VK_RESULT_MAX_ENUM" << std::endl;
				break;
			default:
				break;
		}
		assert(0 && "Vulkan runtime error!");
	}
}
uint32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties * gpuMemoryProperties, const VkMemoryRequirements * memoryRequirements, const VkMemoryPropertyFlags requiredProperties)
{
	uint32_t memoryIndex;
	for (uint32_t i = 0; i < gpuMemoryProperties->memoryTypeCount; i++)
	{
		if (memoryRequirements->memoryTypeBits & (1 << i))
		{
			if ((gpuMemoryProperties->memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties)
			{
				memoryIndex = i;
				break;
			}
		}
	}

	if (memoryIndex == UINT32_MAX) {
		assert(0 && "No compatible memory type found!");
	}
	return memoryIndex;
}
#else
void ErrorCheck(VkResult)
{
}
#endif

VkVertexInputBindingDescription Vertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

	return attributeDescriptions;
}

void CreateBuffer(VkDevice device, VkPhysicalDeviceMemoryProperties deviceMemoryProperties, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	ErrorCheck(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryTypeIndex(&deviceMemoryProperties, &memRequirements, properties);

	ErrorCheck(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void CopyBuffer(VkDevice device,  VkQueue graphicsQueue, VkCommandPool cmdPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, cmdPool);
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(device, graphicsQueue, cmdPool, commandBuffer);
}

void CreateImage(VkDevice device, const VkPhysicalDeviceMemoryProperties& physicalDeviceMemProperties ,uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ErrorCheck(vkCreateImage(device, &imageInfo, nullptr, &image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryTypeIndex(&physicalDeviceMemProperties, &memRequirements, properties);

	ErrorCheck(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));

	vkBindImageMemory(device, image, imageMemory, 0);
};

VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void EndSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);
	
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
} 

void TransitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool cmdPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, cmdPool);
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
	switch (oldLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		// Image layout is undefined (or does not matter)
		// Only valid as initial layout
		// No flags required, listed only for completeness
		barrier.srcAccessMask = 0;
		break;

	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		// Image is preinitialized
		// Only valid as initial layout for linear images, preserves memory contents
		// Make sure host writes have been finished
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image is a color attachment
		// Make sure any writes to the color buffer have been finished
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image is a depth/stencil attachment
		// Make sure any writes to the depth/stencil buffer have been finished
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image is a transfer source 
		// Make sure any reads from the image have been finished
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image is a transfer destination
		// Make sure any writes to the image have been finished
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image is read by a shader
		// Make sure any shader reads from the image have been finished
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (newLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image will be used as a transfer destination
		// Make sure any writes to the image have been finished
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image will be used as a transfer source
		// Make sure any reads from the image have been finished
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image will be used as a color attachment
		// Make sure any writes to the color buffer have been finished
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image layout will be used as a depth/stencil attachment
		// Make sure any writes to depth/stencil buffer have been finished
		barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image will be read in a shader (sampler, input attachment)
		// Make sure any writes to the image have been finished
		if (barrier.srcAccessMask == 0)
		{
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	EndSingleTimeCommands(device, graphicsQueue, cmdPool, commandBuffer);
}

void CopyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool cmdPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) 
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, cmdPool);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0; //means tightly packed
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0,0,0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	EndSingleTimeCommands(device, graphicsQueue, cmdPool, commandBuffer);
}

VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	ErrorCheck(vkCreateImageView(device, &viewInfo, nullptr, &imageView));

	return imageView;
}