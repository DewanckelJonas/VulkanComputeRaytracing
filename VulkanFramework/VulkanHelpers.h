#pragma once
#include "glm/glm.hpp"
#include <array>
enum VkResult;
void ErrorCheck(VkResult result);
void CreateBuffer(VkDevice device, VkPhysicalDeviceMemoryProperties deviceMemoryProperties, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory);
void CopyBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool cmdPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
void CreateImage(VkDevice device, const VkPhysicalDeviceMemoryProperties & physicalDeviceMemProperties, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & imageMemory);
VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
void EndSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer);
void TransitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool cmdPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
void CopyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool cmdPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format);
uint32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties* gpuMemoryProperties, const VkMemoryRequirements* memoryRequirements, const VkMemoryPropertyFlags memoryProperties);
struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

struct Circle
{
	glm::vec3 center;
	float radius;
};

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};