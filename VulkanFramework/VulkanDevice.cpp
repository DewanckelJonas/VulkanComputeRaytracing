#include "VulkanDevice.h"
#include "BUILD_OPTIONS.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <assert.h>
#include "VulkanHelpers.h"
#include "VulkanDevice.h"


using namespace vkw;
VulkanDevice::VulkanDevice()
{
	SetupDebug();
	SetUpLayersAndExtensions();
	InitInstance();
	InitDebug();
	InitDevice();
}


VulkanDevice::~VulkanDevice()
{
	delete m_Window;

	DeInitDevice();
	DeInitDebug();
	DeInitInstance();
}

const VkInstance VulkanDevice::GetInstance() const
{
	return m_pInstance;
}

const VkPhysicalDevice VulkanDevice::GetPhysicalDevice() const
{
	return m_pGPU;
}

const VkDevice VulkanDevice::GetDevice() const
{
	return m_pDevice;
}

const uint32_t VulkanDevice::GetGraphicsFamilyQueueId() const
{
	return m_GraphicsQueueFamilyId;
}

const uint32_t vkw::VulkanDevice::GetComputeFamilyQueueId() const
{
	return m_ComputeQueueFamilyId;
}

const VkQueue VulkanDevice::GetQueue() const
{
	return m_pQueue;
}

const VkPhysicalDeviceProperties& VulkanDevice::GetPhysicalDeviceProperties() const
{
	return m_GPUProperties;
}

const VkPhysicalDeviceMemoryProperties & VulkanDevice::GetPhysicalDeviceMemoryProperties() const
{
	return m_GPUMemoryProperties;
}

const VkPhysicalDeviceFeatures & vkw::VulkanDevice::GetDeviceFeatures() const
{
	return m_Features;
}



void VulkanDevice::SetUpLayersAndExtensions()
{
	//Surface extension
	m_InstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	m_InstanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

	m_DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void VulkanDevice::InitInstance()
{
	VkApplicationInfo ApplicationInfo{};
	ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ApplicationInfo.apiVersion = VK_API_VERSION_1_1;
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	ApplicationInfo.pApplicationName = "Vulkan Framework";



	VkInstanceCreateInfo InstanceCreateInfo{};
	InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	InstanceCreateInfo.pApplicationInfo = &ApplicationInfo;
	InstanceCreateInfo.enabledLayerCount = m_InstanceLayers.size();
	InstanceCreateInfo.ppEnabledLayerNames = m_InstanceLayers.data();
	InstanceCreateInfo.enabledExtensionCount = m_InstanceExtensions.size();
	InstanceCreateInfo.ppEnabledExtensionNames = m_InstanceExtensions.data();
	InstanceCreateInfo.pNext = &m_DebugCallbackCreateInfo;


	ErrorCheck(vkCreateInstance(&InstanceCreateInfo, nullptr, &m_pInstance));
}

void VulkanDevice::DeInitInstance()
{
	vkDestroyInstance(m_pInstance, nullptr);
	m_pInstance = VK_NULL_HANDLE;
}

void VulkanDevice::InitDevice()
{

	GetGPU();

	uint32_t familyCount{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(m_pGPU, &familyCount, nullptr);
	std::vector<VkQueueFamilyProperties> familyPropertiesList(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_pGPU, &familyCount, familyPropertiesList.data());
	
	bool foundGraphics{ false };
	for (uint32_t i = 0; i < familyCount; i++)
	{
		if(familyPropertiesList[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
			foundGraphics = true;
			m_GraphicsQueueFamilyId = i;
		}
	}
	bool foundCompute{ false };
	for (uint32_t i = 0; i < familyCount; i++)
	{
		if (familyPropertiesList[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
			foundCompute = true;
			m_ComputeQueueFamilyId = i;
		}
	}

	if(!foundGraphics)
	{
		assert(0);
		assert(0 && "Vulkan ERROR: Queue family index supporting graphics not found!");
		std::exit(-1);
	}

	if (!foundCompute)
	{
		assert(0 && "Vulkan ERROR: Queue family index supporting Compute not found!");
		std::exit(-1);
	}

	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> layersProperties(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, layersProperties.data());
		std::cout << "Instance Layers: \n";
		for (VkLayerProperties& layerProps : layersProperties)
		{
			std::cout << " " << layerProps.layerName << "\t\t | " << layerProps.description << std::endl;
		}
		std::cout << std::endl;
	}

	{
		uint32_t layerCount = 0;
		vkEnumerateDeviceLayerProperties(m_pGPU, &layerCount, nullptr);
		std::vector<VkLayerProperties> layersProperties(layerCount);
		vkEnumerateDeviceLayerProperties(m_pGPU, &layerCount, layersProperties.data());
		std::cout << "Device Layers (Depricated) : /n";
		for (VkLayerProperties& layerProps : layersProperties)
		{
			std::cout << " " << layerProps.layerName << "\t\t | " << layerProps.description << std::endl;
		}
		std::cout << std::endl;
	}

	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos{};
	if (m_ComputeQueueFamilyId != m_GraphicsQueueFamilyId)
	{
		VkDeviceQueueCreateInfo computeQueueCreateInfo{};
		computeQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		computeQueueCreateInfo.queueFamilyIndex = m_ComputeQueueFamilyId;
		computeQueueCreateInfo.queueCount = 1;
		float queuePriorities[1] = { 0.0 };
		computeQueueCreateInfo.pQueuePriorities = queuePriorities;
		deviceQueueCreateInfos.push_back(computeQueueCreateInfo);
	}
	VkDeviceQueueCreateInfo graphicsQueueCreateInfo {};
	graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphicsQueueCreateInfo.queueFamilyIndex = m_GraphicsQueueFamilyId;
	graphicsQueueCreateInfo.queueCount = 1;
	float queuePriorities[1] = { 0.0 };
	graphicsQueueCreateInfo.pQueuePriorities = queuePriorities;
	deviceQueueCreateInfos.push_back(graphicsQueueCreateInfo);

	VkDeviceCreateInfo deviceCreateInfo {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = deviceQueueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
	deviceCreateInfo.enabledLayerCount = m_DeviceLayers.size();
	deviceCreateInfo.ppEnabledLayerNames = m_DeviceLayers.data();
	deviceCreateInfo.enabledExtensionCount = m_DeviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

	ErrorCheck(vkCreateDevice(m_pGPU, &deviceCreateInfo, nullptr, &m_pDevice));

	vkGetDeviceQueue(m_pDevice, m_GraphicsQueueFamilyId, 0, &m_pQueue);
}

void VulkanDevice::DeInitDevice()
{
	vkDestroyDevice(m_pDevice, nullptr);
	m_pDevice = VK_NULL_HANDLE;
}

void VulkanDevice::GetGPU()
{
	uint32_t GPUCount = 0;
	vkEnumeratePhysicalDevices(m_pInstance, &GPUCount, VK_NULL_HANDLE);
	std::vector<VkPhysicalDevice> GPUVec{ GPUCount };
	vkEnumeratePhysicalDevices(m_pInstance, &GPUCount, GPUVec.data());
	m_pGPU = GPUVec[0];
	vkGetPhysicalDeviceProperties(m_pGPU, &m_GPUProperties);
	vkGetPhysicalDeviceFeatures(m_pGPU, &m_Features);
	vkGetPhysicalDeviceMemoryProperties(m_pGPU, &m_GPUMemoryProperties);
}

#if BUILD_ENABLE_VULKAN_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType, 
	uint64_t sourceObj,
	size_t location,
	int32_t msgCode,
	const char* layerPrefix,
	const char* message,
	void* userData
)
{
	std::ostringstream stream;
	std::cout << "VKDBG: ";
	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
	{
		stream << "INFO: ";
	}
	if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		stream << "WARNING: ";
	}
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		stream << "PERFORMANCE: ";
	}
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		stream << "ERROR: ";
	}
	if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		stream << "DEBUG: ";
	}

	stream << "@[" << layerPrefix << "]: ";
	stream << message << std::endl;
	std::cout << stream.str();

#ifdef _WIN32
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		MessageBox(NULL, stream.str().c_str(), "Vulkan Error!", 0);
	}
#endif

	return false;
}
// ObjType : the object which is the cause of the error not where it happens f.e. when you destroy a device without destroying the command pool the command pool will be the object type because its causing a memory leak.


void VulkanDevice::SetupDebug()
{

	m_InstanceLayers.push_back("VK_LAYER_LUNARG_standard_validation"); //recommended

	////Custom Layer order not recommended 
	//m_InstanceLayers.push_back("VK_LAYER_GOOGLE_threading");
	//m_InstanceLayers.push_back("VK_LAYER_LUNARG_parameter_validation");
	//m_InstanceLayers.push_back("VK_LAYER_LUNARG_object_tracker");
	//m_InstanceLayers.push_back("VK_LAYER_LUNARG_screenshot");
	//m_InstanceLayers.push_back("VK_LAYER_LUNARG_core_validation");
	////m_InstanceLayers.push_back("VK_LAYER_LUNARG_swapchain");
	//m_InstanceLayers.push_back("VK_LAYER_GOOGLE_unique_objects");

	m_InstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
}

PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT = nullptr; //f stands for fetched function
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT = nullptr;


void VulkanDevice::InitDebug()
{
	fvkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_pInstance, "vkCreateDebugReportCallbackEXT"));
	fvkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_pInstance, "vkDestroyDebugReportCallbackEXT"));
	if(nullptr == fvkCreateDebugReportCallbackEXT || nullptr == fvkDestroyDebugReportCallbackEXT)
	{
		assert(0 && "Vulkan ERROR: Can't fetch debug function pointers.");
		std::exit(-1);
	}
	
	m_DebugCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	m_DebugCallbackCreateInfo.pfnCallback = VulkanDebugCallback;
	m_DebugCallbackCreateInfo.flags =
		//VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
		//VK_DEBUG_REPORT_DEBUG_BIT_EXT |;
		0;

	fvkCreateDebugReportCallbackEXT(m_pInstance, &m_DebugCallbackCreateInfo, nullptr, &m_DebugReport);
}

void VulkanDevice::DeInitDebug()
{
	fvkDestroyDebugReportCallbackEXT(m_pInstance, m_DebugReport, nullptr);
	m_DebugReport = VK_NULL_HANDLE;
}

#else

void VulkanBase::SetupDebug() {};
void VulkanBase::InitDebug() {};
void VulkanBase::DeInitDebug() {};


#endif

