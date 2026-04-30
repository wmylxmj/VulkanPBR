#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

struct GlobalConfig {
	void* hWnd;
	uint32_t viewportWidth;
	uint32_t viewportHeight;
	VkInstance vulkanInstance;
	VkSurfaceKHR vulkanSurface;
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	int graphicsQueueFamilyIndex;
	int presentQueueFamilyIndex;
	VkSampleCountFlagBits physicalDeviceMaxSampleCount;
	VkSampleCountFlagBits preferredSampleCount;
	char** enabledLayers;
	int enabledLayerCount;
};

bool InitVulkan(void* param, int width, int height);
