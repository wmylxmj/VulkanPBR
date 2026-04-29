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

	return true;
}