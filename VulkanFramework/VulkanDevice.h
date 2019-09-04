#pragma once

#include "Platform.h"
#include <vector>
namespace vkw
{
	class Window;

	class VulkanDevice
	{
	public:
		VulkanDevice();
		~VulkanDevice();

		const VkInstance GetInstance() const;
		const VkPhysicalDevice GetPhysicalDevice() const;
		const VkDevice GetDevice() const;
		const uint32_t GetGraphicsFamilyQueueId() const;
		const uint32_t GetComputeFamilyQueueId() const;
		const VkQueue GetQueue() const;
		const  VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() const;
		const VkPhysicalDeviceMemoryProperties & GetPhysicalDeviceMemoryProperties() const;

	private:
		void SetUpLayersAndExtensions();

		void InitInstance();
		void DeInitInstance();

		void InitDevice();
		void DeInitDevice();

		void SetupDebug();
		void InitDebug();
		void DeInitDebug();

		void GetGPU();

		VkInstance m_pInstance = VK_NULL_HANDLE; // we use null handle instead of nullptr because null handle has the same size in x64 and x86
		VkPhysicalDevice m_pGPU = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_GPUProperties{};
		VkPhysicalDeviceMemoryProperties m_GPUMemoryProperties{};
		VkDevice m_pDevice = VK_NULL_HANDLE;
		VkQueue m_pQueue = VK_NULL_HANDLE;

		uint32_t m_GraphicsQueueFamilyId = 0;
		uint32_t m_ComputeQueueFamilyId = 0;

		Window* m_Window;

		std::vector<const char*> m_InstanceLayers;
		std::vector<const char*> m_InstanceExtensions;
		std::vector<const char*> m_DeviceLayers;
		std::vector<const char*> m_DeviceExtensions;

		VkDebugReportCallbackEXT m_DebugReport = VK_NULL_HANDLE;
		VkDebugReportCallbackCreateInfoEXT m_DebugCallbackCreateInfo{};
	};
}

