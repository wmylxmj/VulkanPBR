#include "VulkanUtils.h"

#include <string>

static GlobalConfig s_globalConfig = {};

static const char* s_enabledExtensions[] = {
	VK_KHR_SURFACE_EXTENSION_NAME ,
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME
}; // For vulkan instance

static PFN_vkCreateDebugReportCallbackEXT __vkCreateDebugReportCallback = nullptr;
static PFN_vkDestroyDebugReportCallbackEXT __vkDestroyDebugReportCallback = nullptr;
static PFN_vkCreateWin32SurfaceKHR __vkCreateWin32SurfaceKHR = nullptr;

static VkDebugReportCallbackEXT s_vulkanDebugReportCallback = nullptr;

static void InitvalidationLayers() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	if (layerCount == 0) {
		return;
	}
	VkLayerProperties* layers = new VkLayerProperties[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, layers);
	int enabledLayerCount = 0;
	for (uint32_t i = 0; i < layerCount; ++i) {
		if (nullptr != strstr(layers[i].layerName, "validation")) {
			enabledLayerCount++;
		}
	}
	s_globalConfig.enabledLayerCount = enabledLayerCount;
	if (enabledLayerCount > 0) {
		s_globalConfig.enabledLayers = new char* [enabledLayerCount];
		int enabledLayerIndex = 0;
		for (uint32_t i = 0; i < layerCount; i++) {
			if (nullptr != strstr(layers[i].layerName, "validation")) {
				s_globalConfig.enabledLayers[enabledLayerIndex] = new char[64];
				memset(s_globalConfig.enabledLayers[enabledLayerIndex], 0, 64);
				strcpy_s(s_globalConfig.enabledLayers[enabledLayerIndex], 64, layers[i].layerName);
				OutputDebugStringA(
					(
						std::string("InitvalidationLayers enable ") +
						std::string(s_globalConfig.enabledLayers[enabledLayerIndex]) +
						std::string("\n")
						).c_str()
				);
				enabledLayerIndex++;
			}
		}
	}
	delete[] layers;
}

static bool InitVulkanInstance() {
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "VulkanPBR";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "MiniEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = sizeof(s_enabledExtensions) / sizeof(s_enabledExtensions[0]);
	createInfo.ppEnabledExtensionNames = s_enabledExtensions;
	InitvalidationLayers();
	createInfo.enabledLayerCount = s_globalConfig.enabledLayerCount;
	createInfo.ppEnabledLayerNames = s_globalConfig.enabledLayers;
	if (vkCreateInstance(&createInfo, nullptr, &s_globalConfig.vulkanInstance) != VK_SUCCESS) {
		OutputDebugStringA("Failed to create vulkan instance\n");
		return false;
	}
	return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData
) {
	OutputDebugStringA(pMessage);
	return VK_FALSE;
}

static bool InitDebugReportCallback() {
	VkDebugReportCallbackCreateInfoEXT createDebugInfo = {};
	createDebugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createDebugInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createDebugInfo.pfnCallback = DebugCallback;
	if (__vkCreateDebugReportCallback(
		s_globalConfig.vulkanInstance,
		&createDebugInfo,
		nullptr,
		&s_vulkanDebugReportCallback
	) != VK_SUCCESS) {
		return false;
	}
	return true;
}

static bool InitSurface() {
	VkWin32SurfaceCreateInfoKHR createSurfaceInfo;
	createSurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createSurfaceInfo.hwnd = (HWND)s_globalConfig.hWnd;
	createSurfaceInfo.hinstance = GetModuleHandle(nullptr);
	createSurfaceInfo.pNext = nullptr;
	createSurfaceInfo.flags = 0;
	if (__vkCreateWin32SurfaceKHR(
		s_globalConfig.vulkanInstance,
		&createSurfaceInfo,
		nullptr,
		&s_globalConfig.vulkanSurface
	) != VK_SUCCESS) {
		return false;
	}
	return true;
}

static VkSampleCountFlagBits GetPhysicalDeviceMaxSampleCount(VkPhysicalDevice physicalDevice) {
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	VkSampleCountFlags sampleCountFlags = physicalDeviceProperties.limits.framebufferColorSampleCounts > physicalDeviceProperties.limits.framebufferDepthSampleCounts ?
		physicalDeviceProperties.limits.framebufferDepthSampleCounts : physicalDeviceProperties.limits.framebufferColorSampleCounts;
	if (sampleCountFlags & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (sampleCountFlags & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (sampleCountFlags & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (sampleCountFlags & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (sampleCountFlags & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (sampleCountFlags & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

static bool InitPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(s_globalConfig.vulkanInstance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		OutputDebugStringA("Cannot find any gpu on this computer!\n");
		return false;
	}
	VkPhysicalDevice* devices = new VkPhysicalDevice[deviceCount];
	vkEnumeratePhysicalDevices(s_globalConfig.vulkanInstance, &deviceCount, devices);

	for (uint32_t i = 0; i < deviceCount; ++i) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
		vkGetPhysicalDeviceFeatures(devices[i], &deviceFeatures);
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, nullptr);
		VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)alloca(queueFamilyCount * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies);
		s_globalConfig.graphicsQueueFamilyIndex = -1;
		s_globalConfig.presentQueueFamilyIndex = -1;
		for (uint32_t j = 0; j < queueFamilyCount; ++j) {
			if (queueFamilies[j].queueCount > 0 && queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				s_globalConfig.graphicsQueueFamilyIndex = j;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, s_globalConfig.vulkanSurface, &presentSupport);
			if (queueFamilies[j].queueCount > 0 && presentSupport) {
				s_globalConfig.presentQueueFamilyIndex = j;
			}
			if (s_globalConfig.graphicsQueueFamilyIndex != -1 && s_globalConfig.presentQueueFamilyIndex != -1) {
				s_globalConfig.physicalDevice = devices[i];
				s_globalConfig.physicalDeviceMaxSampleCount = GetPhysicalDeviceMaxSampleCount(devices[i]);
				if (s_globalConfig.preferredSampleCount > s_globalConfig.physicalDeviceMaxSampleCount) {
					s_globalConfig.preferredSampleCount = s_globalConfig.physicalDeviceMaxSampleCount;
				}
				return true;
			}
		}
	}
	return false;
}

bool InitVulkan(void* param, int width, int height)
{
	s_globalConfig.hWnd = param;
	s_globalConfig.viewportWidth = width;
	s_globalConfig.viewportHeight = height;
	if (!InitVulkanInstance()) {
		return false;
	}
	__vkCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(s_globalConfig.vulkanInstance, "vkCreateDebugReportCallbackEXT");
	__vkDestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(s_globalConfig.vulkanInstance, "vkDestroyDebugReportCallbackEXT");
	__vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(s_globalConfig.vulkanInstance, "vkCreateWin32SurfaceKHR");
	if (!InitDebugReportCallback()) {
		return false;
	}
	if (!InitSurface()) {
		return false;
	}
	if (!InitPhysicalDevice()) {
		return false;
	}

	return true;
}