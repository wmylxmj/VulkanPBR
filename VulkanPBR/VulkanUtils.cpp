#include "VulkanUtils.h"

#include <string>
#include <vector>
#include <set>

static GlobalConfig s_globalConfig = {};

static const char* s_enabledExtensions[] = {
	VK_KHR_SURFACE_EXTENSION_NAME ,
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME
}; // For vulkan instance
static const char* s_enabledLogicDeviceExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_EXT_NON_SEAMLESS_CUBE_MAP_EXTENSION_NAME
};

static PFN_vkCreateDebugReportCallbackEXT __vkCreateDebugReportCallback = nullptr;
static PFN_vkDestroyDebugReportCallbackEXT __vkDestroyDebugReportCallback = nullptr;
static PFN_vkCreateWin32SurfaceKHR __vkCreateWin32SurfaceKHR = nullptr;

static VkDebugReportCallbackEXT s_vulkanDebugReportCallback = nullptr;

static uint32_t s_nextSystemFrameBufferToRender = -1;

Texture::Texture(VkFormat inFormat, VkImageAspectFlags inImageAspectFlag)
{
	image = nullptr;
	memory = nullptr;
	imageView = nullptr;
	format = inFormat;
	imageAspectFlag = inImageAspectFlag;
}

Texture::~Texture()
{
	if (memory != nullptr) {
		vkFreeMemory(s_globalConfig.logicalDevice, memory, nullptr);
	}
	if (imageView != nullptr) {
		vkDestroyImageView(s_globalConfig.logicalDevice, imageView, nullptr);
	}
	if (image != nullptr) {
		vkDestroyImage(s_globalConfig.logicalDevice, image, nullptr);
	}
}

FrameBuffer::FrameBuffer()
{
	frameBuffer = nullptr;
	depthAttachment = nullptr;
	resolveBuffer = nullptr;
	sampleCount = VK_SAMPLE_COUNT_1_BIT;
}

FrameBuffer::~FrameBuffer()
{
	if (depthAttachment != nullptr) {
		delete depthAttachment;
		depthAttachment = nullptr;
	}
	for (Texture* colorAttachment : colorAttachments) {
		delete colorAttachment;
	}
	colorAttachments.clear();
}

void BufferObject::Write(void* inData, int inDataSize)
{
	void* data = nullptr;
	if (inDataSize == 0) {
		inDataSize = size;
	}
	MapMemory(memory, 0, inDataSize, 0, &data);
	memcpy(data, inData, inDataSize);
	UnmapMemory(memory);
}

Mat4UniformBufferData::Mat4UniformBufferData(int inMat4Count)
{
	mat4Count = inMat4Count;
	bNeedSyncData = true;
	ubo = 0;
}

void Mat4UniformBufferData::SetMat4(int inIndex, float* inMat4) {
	int offset = inIndex * 16;
	memcpy(data + offset, inMat4, sizeof(float) * 16);
	bNeedSyncData = true;
}

void Mat4UniformBufferData::UpdateGPUData() {
	if (!bNeedSyncData) {
		return;
	}
	if (ubo == nullptr) {
		ubo = CreateBuffer(
			sizeof(data),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
	}
	bNeedSyncData = false;
	ubo->Write(data, sizeof(data));
}

std::vector<VkDescriptorSetLayoutBinding> UniformInputsBindings::descriptorSetLayoutBindings;
std::vector<VkDescriptorPoolSize> UniformInputsBindings::descriptorPoolSizes;
VkDescriptorSetLayout UniformInputsBindings::descriptorSetLayout;
int UniformInputsBindings::descriptorSetLayoutCount = 1;

void UniformInputsBindings::Init()
{
	InitUniformInput(0, VK_SHADER_STAGE_VERTEX_BIT);
	InitUniformInput(1, VK_SHADER_STAGE_VERTEX_BIT);
	InitUniformInput(2, VK_SHADER_STAGE_VERTEX_BIT);
	InitUniformInput(3, VK_SHADER_STAGE_FRAGMENT_BIT);
	InitUniformInput(4, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(5, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(6, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(7, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(8, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(9, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(10, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(11, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(12, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
	layoutInfo.pBindings = descriptorSetLayoutBindings.data();
	if (vkCreateDescriptorSetLayout(s_globalConfig.logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		OutputDebugStringA("Failed to create descriptor set layout!\n");
	}
}

void UniformInputsBindings::InitUniformInput(int inBindingPoint, VkShaderStageFlags inVkShaderStageFlags, VkDescriptorType inVkDescriptorType)
{
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
	descriptorSetLayoutBinding.binding = inBindingPoint;
	descriptorSetLayoutBinding.descriptorType = inVkDescriptorType;
	descriptorSetLayoutBinding.descriptorCount = 1;
	descriptorSetLayoutBinding.stageFlags = inVkShaderStageFlags;
	descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
	descriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);

	VkDescriptorPoolSize descriptorPoolSize = {};
	descriptorPoolSize.type = inVkDescriptorType;
	descriptorPoolSize.descriptorCount = 1;
	descriptorPoolSizes.push_back(descriptorPoolSize);
}

VkPipelineLayout PipelineStateObject::pipelineLayout = nullptr;

void PipelineStateObject::Init()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pSetLayouts = &UniformInputsBindings::descriptorSetLayout;
	pipelineLayoutInfo.setLayoutCount = UniformInputsBindings::descriptorSetLayoutCount;
	if (vkCreatePipelineLayout(s_globalConfig.logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		OutputDebugStringA("Failed to create pipeline layout\n");
	}
}

PipelineStateObject::PipelineStateObject() : pipeline(nullptr)
{
	viewport = {};
	scissor = {};

	colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY;
	colorBlendState.attachmentCount = 0;
	colorBlendState.pAttachments = nullptr;
	colorBlendState.blendConstants[0] = 0.0f;
	colorBlendState.blendConstants[1] = 0.0f;
	colorBlendState.blendConstants[2] = 0.0f;
	colorBlendState.blendConstants[3] = 0.0f;

	renderPass = nullptr;
	sampleCount = VK_SAMPLE_COUNT_1_BIT;

	depthConstantFactor = 0.0f;
	depthClamp = 0.0f;
	depthSlopeFactor = 0.0f;
}

PipelineStateObject::~PipelineStateObject()
{
	CleanUp();
}

void PipelineStateObject::CleanUp()
{
	if (pipeline != nullptr) {
		vkDestroyPipeline(s_globalConfig.logicalDevice, pipeline, nullptr);
		pipeline = nullptr;
	}
}

uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(s_globalConfig.physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return 0;
}

void GenImage(Texture* texture, uint32_t width, uint32_t height, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount, int mipmaplevel)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent = { width, height, 1 };
	imageInfo.mipLevels = mipmaplevel;
	imageInfo.arrayLayers = 1;
	imageInfo.format = texture->format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = sampleCount;
	if (vkCreateImage(s_globalConfig.logicalDevice, &imageInfo, nullptr, &texture->image) != VK_SUCCESS) {
		OutputDebugStringA("Failed to create image!\n");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(s_globalConfig.logicalDevice, texture->image, &memRequirements);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (vkAllocateMemory(s_globalConfig.logicalDevice, &allocInfo, nullptr, &texture->memory) != VK_SUCCESS) {
		OutputDebugStringA("Failed to allocate image memory!\n");
	}
	vkBindImageMemory(s_globalConfig.logicalDevice, texture->image, texture->memory, 0);
}

VkImageView GenImageView2D(VkImage inImage, VkFormat inFormat, VkImageAspectFlags inImageAspectFlags, int inMipmapLevelCount) {
	VkImageView imageView;
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = inImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = inFormat;
	viewInfo.subresourceRange.aspectMask = inImageAspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = inMipmapLevelCount;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	if (vkCreateImageView(s_globalConfig.logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		printf("failed to create texture image view!");
		return nullptr;
	}
	return imageView;
}

void TransferImageLayout(
	VkCommandBuffer inCommandBuffer, VkImage inImage, VkImageSubresourceRange inSubresourceRange,
	VkImageLayout inOldLayout, VkAccessFlags inOldAccessFlags, VkPipelineStageFlags inSrcStageMask,
	VkImageLayout inNewLayout, VkAccessFlags inNewAccessFlags, VkPipelineStageFlags inDstStageMask
) {
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.oldLayout = inOldLayout;
	barrier.srcAccessMask = inNewAccessFlags;
	barrier.dstAccessMask = inDstStageMask;
	barrier.newLayout = inNewLayout;
	barrier.image = inImage;
	barrier.subresourceRange = inSubresourceRange;
	vkCmdPipelineBarrier(inCommandBuffer, inSrcStageMask, inDstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

GlobalConfig& GetGlobalConfig()
{
	return s_globalConfig;
}

static void InitValidationLayers() {
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
	InitValidationLayers();
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

static void InitVulkanLogicalDevice() {
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = {
		s_globalConfig.graphicsQueueFamilyIndex,
		s_globalConfig.presentQueueFamilyIndex
	};
	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = VK_TRUE;

	VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT nonSeamlessCubeMapFeatures = {};
	nonSeamlessCubeMapFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NON_SEAMLESS_CUBE_MAP_FEATURES_EXT;
	nonSeamlessCubeMapFeatures.nonSeamlessCubeMap = VK_TRUE;

	VkDeviceCreateInfo createDeviceInfo = {};
	createDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createDeviceInfo.pQueueCreateInfos = queueCreateInfos.data();
	createDeviceInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();

	createDeviceInfo.pEnabledFeatures = &deviceFeatures;
	createDeviceInfo.enabledExtensionCount = 0;

	createDeviceInfo.enabledLayerCount = s_globalConfig.enabledLayerCount;
	createDeviceInfo.ppEnabledLayerNames = s_globalConfig.enabledLayers;

	createDeviceInfo.enabledExtensionCount = sizeof(s_enabledLogicDeviceExtensions) / sizeof(s_enabledLogicDeviceExtensions[0]);
	createDeviceInfo.ppEnabledExtensionNames = s_enabledLogicDeviceExtensions;
	createDeviceInfo.pNext = &nonSeamlessCubeMapFeatures;

	if (vkCreateDevice(
		s_globalConfig.physicalDevice,
		&createDeviceInfo,
		nullptr,
		&s_globalConfig.logicalDevice
	) != VK_SUCCESS) {
		OutputDebugStringA("Failed to create vulkan logical device\n");
	}
	vkGetDeviceQueue(s_globalConfig.logicalDevice, s_globalConfig.graphicsQueueFamilyIndex, 0, &s_globalConfig.graphicsQueue);
	vkGetDeviceQueue(s_globalConfig.logicalDevice, s_globalConfig.presentQueueFamilyIndex, 0, &s_globalConfig.presentQueue);
}

static void InitSwapchainDetailedInfo() {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s_globalConfig.physicalDevice, s_globalConfig.vulkanSurface, &s_globalConfig.surfaceCapabilities);
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(s_globalConfig.physicalDevice, s_globalConfig.vulkanSurface, &formatCount, nullptr);

	if (formatCount != 0) {
		s_globalConfig.surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(s_globalConfig.physicalDevice, s_globalConfig.vulkanSurface, &formatCount, s_globalConfig.surfaceFormats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(s_globalConfig.physicalDevice, s_globalConfig.vulkanSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		s_globalConfig.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(s_globalConfig.physicalDevice, s_globalConfig.vulkanSurface, &presentModeCount, s_globalConfig.presentModes.data());
	}
}

void InitSwapchain() {
	InitSwapchainDetailedInfo();
	VkSurfaceFormatKHR surfaceFormat;
	for (const auto& availableFormat : s_globalConfig.surfaceFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			surfaceFormat.colorSpace = availableFormat.colorSpace;
			surfaceFormat.format = availableFormat.format;
			break;
		}
	}
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	for (const auto& availablePresentMode : s_globalConfig.presentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
			presentMode = availablePresentMode;
			break;
		}
	}
	VkExtent2D extent = { uint32_t(s_globalConfig.viewportWidth), uint32_t(s_globalConfig.viewportHeight) };

	uint32_t imageCount = s_globalConfig.surfaceCapabilities.minImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = s_globalConfig.vulkanSurface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	OutputDebugStringA((std::string("Swapchain image count: ") + std::to_string(imageCount) + "\n").c_str());
	uint32_t queueFamilyIndices[] = { (uint32_t)s_globalConfig.graphicsQueueFamilyIndex, (uint32_t)s_globalConfig.presentQueueFamilyIndex };

	if (s_globalConfig.graphicsQueueFamilyIndex != s_globalConfig.presentQueueFamilyIndex) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = s_globalConfig.surfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(s_globalConfig.logicalDevice, &createInfo, nullptr, &s_globalConfig.swapchain) != VK_SUCCESS) {
		OutputDebugStringA("Failed to create swap chain!\n");
	}

	vkGetSwapchainImagesKHR(s_globalConfig.logicalDevice, s_globalConfig.swapchain, &imageCount, nullptr);
	s_globalConfig.swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(s_globalConfig.logicalDevice, s_globalConfig.swapchain, &imageCount, s_globalConfig.swapchainImages.data());

	s_globalConfig.swapchainImageFormat = surfaceFormat.format;
	s_globalConfig.swapchainExtent = extent;
	s_globalConfig.swapchainImageViews.resize(imageCount);
	s_globalConfig.systemFrameBufferCount = imageCount;
	for (uint32_t i = 0; i < imageCount; i++) {
		s_globalConfig.swapchainImageViews[i] = GenImageView2D(s_globalConfig.swapchainImages[i], s_globalConfig.swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

static void InitSystemRenderPass() {
	VkSampleCountFlagBits sampleCount = s_globalConfig.preferredSampleCount;
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = s_globalConfig.swapchainImageFormat;
	colorAttachment.samples = sampleCount;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (sampleCount & VK_SAMPLE_COUNT_1_BIT) {
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
	else {
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = VK_FORMAT_D24_UNORM_S8_UINT;
	depthAttachment.samples = sampleCount;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription attachments[3];
	int attachment_count = 2;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	if (sampleCount & VK_SAMPLE_COUNT_1_BIT) {
		memcpy(&attachments[0], &colorAttachment, sizeof(VkAttachmentDescription));
		memcpy(&attachments[1], &depthAttachment, sizeof(VkAttachmentDescription));
	}
	else {
		VkAttachmentDescription colorAttachmentResolve = {};
		colorAttachmentResolve.format = s_globalConfig.swapchainImageFormat;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentResolveRef = {};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;
		attachment_count = 3;
		memcpy(&attachments[0], &colorAttachment, sizeof(VkAttachmentDescription));
		memcpy(&attachments[1], &depthAttachment, sizeof(VkAttachmentDescription));
		memcpy(&attachments[2], &colorAttachmentResolve, sizeof(VkAttachmentDescription));
	}

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachment_count;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(s_globalConfig.logicalDevice, &renderPassInfo, nullptr, &s_globalConfig.systemRenderPass) != VK_SUCCESS) {
		OutputDebugStringA("Failed to create global render pass\n");
	}
}

static void SystemFrameBufferFinish(int inIndex, FrameBuffer& framebuffer) {
	VkImageView attachments[3];
	int attachmentCount = 2;
	if (s_globalConfig.preferredSampleCount == VK_SAMPLE_COUNT_1_BIT) {
		attachments[0] = s_globalConfig.swapchainImageViews[inIndex];
		attachments[1] = framebuffer.depthAttachment->imageView;
	}
	else {
		attachmentCount = 3;
		attachments[0] = framebuffer.colorAttachments[0]->imageView;
		attachments[1] = framebuffer.depthAttachment->imageView;
		attachments[2] = s_globalConfig.swapchainImageViews[inIndex];
	}
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = s_globalConfig.systemRenderPass;
	framebufferInfo.pAttachments = attachments;
	framebufferInfo.attachmentCount = attachmentCount;
	framebufferInfo.width = s_globalConfig.viewportWidth;
	framebufferInfo.height = s_globalConfig.viewportHeight;
	framebufferInfo.layers = 1;
	if (vkCreateFramebuffer(s_globalConfig.logicalDevice, &framebufferInfo, nullptr, &framebuffer.frameBuffer) != VK_SUCCESS) {
		OutputDebugStringA("Failed to create system frame buffer\n");
	}
}

static void InitSystemFrameBuffer() {
	if (s_globalConfig.systemRenderPass != nullptr) {
		vkDestroyRenderPass(s_globalConfig.logicalDevice, s_globalConfig.systemRenderPass, nullptr);
		s_globalConfig.systemRenderPass = nullptr;
	}
	InitSystemRenderPass();
	if (s_globalConfig.systemFrameBuffers != nullptr) {
		delete[] s_globalConfig.systemFrameBuffers;
	}
	s_globalConfig.systemFrameBuffers = new FrameBuffer[s_globalConfig.systemFrameBufferCount];

	for (int i = 0; i < s_globalConfig.systemFrameBufferCount; i++) {
		Texture* colorBuffer = new Texture(s_globalConfig.swapchainImageFormat);
		GenImage(colorBuffer, s_globalConfig.viewportWidth, s_globalConfig.viewportHeight, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, s_globalConfig.preferredSampleCount);
		colorBuffer->imageView = GenImageView2D(colorBuffer->image, colorBuffer->format, colorBuffer->imageAspectFlag);
		s_globalConfig.systemFrameBuffers[i].colorAttachments.push_back(colorBuffer);

		Texture* depthBuffer = new Texture(VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT);
		GenImage(depthBuffer, s_globalConfig.viewportWidth, s_globalConfig.viewportHeight, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, s_globalConfig.preferredSampleCount);
		depthBuffer->imageView = GenImageView2D(depthBuffer->image, depthBuffer->format, depthBuffer->imageAspectFlag);
		s_globalConfig.systemFrameBuffers[i].depthAttachment = depthBuffer;

		SystemFrameBufferFinish(i, s_globalConfig.systemFrameBuffers[i]);
	}
}

static void InitCommandPool() {
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = s_globalConfig.graphicsQueueFamilyIndex;
	poolInfo.flags = 0;
	if (vkCreateCommandPool(s_globalConfig.logicalDevice, &poolInfo, nullptr, &s_globalConfig.commandPool) != VK_SUCCESS) {
		OutputDebugStringA("Failed to create command pool\n");
	}
}

void InitSemaphores() {
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(s_globalConfig.logicalDevice, &semaphoreInfo, nullptr, &s_globalConfig.readyToRenderSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(s_globalConfig.logicalDevice, &semaphoreInfo, nullptr, &s_globalConfig.readyToPresentSemaphore) != VK_SUCCESS) {
		printf("Failed to create semaphore\n");
	}
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
	InitVulkanLogicalDevice();
	InitSwapchain();
	InitSystemFrameBuffer();
	InitCommandPool();
	InitSemaphores();
	UniformInputsBindings::Init();
	PipelineStateObject::Init();

	return true;
}

VkResult GenCommandBuffer(VkCommandBuffer* commandBuffer, int count, VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = level;
	allocInfo.commandPool = s_globalConfig.commandPool;
	allocInfo.commandBufferCount = count;
	allocInfo.pNext = nullptr;
	return vkAllocateCommandBuffers(s_globalConfig.logicalDevice, &allocInfo, commandBuffer);
}

void DeleteCommandBuffer(VkCommandBuffer* commandBuffer, int count)
{
	vkFreeCommandBuffers(s_globalConfig.logicalDevice, s_globalConfig.commandPool, count, commandBuffer);
}

static VkFramebuffer AquireRenderTarget() {
	vkAcquireNextImageKHR(
		s_globalConfig.logicalDevice,
		s_globalConfig.swapchain,
		UINT64_MAX,
		s_globalConfig.readyToRenderSemaphore,
		VK_NULL_HANDLE,
		&s_nextSystemFrameBufferToRender
	);
	return s_globalConfig.systemFrameBuffers[s_nextSystemFrameBufferToRender].frameBuffer;
}

VkCommandBuffer BeginRendering(VkCommandBuffer inCommandBuffer)
{
	VkCommandBuffer commandbuffer;
	if (inCommandBuffer != nullptr) {
		commandbuffer = inCommandBuffer;
	}
	else {
		BeginOneTimeCommandBuffer(&commandbuffer);
	}
	VkFramebuffer render_target = AquireRenderTarget();
	VkRenderPass render_pass = s_globalConfig.systemRenderPass;
	VkClearValue clearValues[2] = {};
	clearValues[0].color = { 0.1f,0.4f,0.6f,1.0f };
	clearValues[1].depthStencil = { 1.0f, 0u };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.framebuffer = render_target;
	renderPassInfo.renderPass = render_pass;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = { uint32_t(s_globalConfig.viewportWidth),uint32_t(s_globalConfig.viewportHeight) };
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(commandbuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	s_globalConfig.systemRenderPassCommandBuffer = commandbuffer;
	return commandbuffer;
}

void EndRendering()
{
	vkCmdEndRenderPass(s_globalConfig.systemRenderPassCommandBuffer);
	vkEndCommandBuffer(s_globalConfig.systemRenderPassCommandBuffer);
}

VkResult BeginOneTimeCommandBuffer(VkCommandBuffer* commandBuffer)
{
	VkResult ret = GenCommandBuffer(commandBuffer, 1);
	if (ret != VK_SUCCESS) {
		return ret;
	}
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pNext = nullptr;
	beginInfo.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(*commandBuffer, &beginInfo);
	return ret;
}

void WaitForCommmandFinish(VkCommandBuffer commandBuffer)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;
	VkFence fence;
	vkCreateFence(s_globalConfig.logicalDevice, &fenceCreateInfo, nullptr, &fence);
	vkQueueSubmit(s_globalConfig.graphicsQueue, 1, &submitInfo, fence);

	vkWaitForFences(s_globalConfig.logicalDevice, 1, &fence, VK_TRUE, 100000000000);
	vkDestroyFence(s_globalConfig.logicalDevice, fence, nullptr);
}

VkResult EndOneTimeCommandBuffer(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);
	WaitForCommmandFinish(commandBuffer);
	vkFreeCommandBuffers(s_globalConfig.logicalDevice, s_globalConfig.commandPool, 1, &commandBuffer);
	return VK_SUCCESS;
}

VkResult GenBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkResult ret = vkCreateBuffer(s_globalConfig.logicalDevice, &bufferInfo, nullptr, &buffer);
	if (ret != VK_SUCCESS) {
		OutputDebugStringA("Failed to create buffer!\n");
		return ret;
	}
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(s_globalConfig.logicalDevice, buffer, &memRequirements);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
	ret = vkAllocateMemory(s_globalConfig.logicalDevice, &allocInfo, nullptr, &bufferMemory);
	if (ret != VK_SUCCESS) {
		OutputDebugStringA("Failed to allocate buffer memory!");
		return ret;
	}
	vkBindBufferMemory(s_globalConfig.logicalDevice, buffer, bufferMemory, 0);
	return VK_SUCCESS;
}

BufferObject* CreateBuffer(VkDeviceSize inVkDeviceSize, VkBufferUsageFlags inVkBufferUsageFlags, VkMemoryPropertyFlags inVkMemoryPropertyFlags)
{
	BufferObject* bufferObject = new BufferObject();
	bufferObject->size = inVkDeviceSize;
	GenBuffer(bufferObject->buffer, bufferObject->memory, inVkDeviceSize, inVkBufferUsageFlags, inVkMemoryPropertyFlags);
	return bufferObject;
}

void MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData)
{
	vkMapMemory(s_globalConfig.logicalDevice, memory, offset, size, flags, ppData);
}

void UnmapMemory(VkDeviceMemory memory)
{
	vkUnmapMemory(s_globalConfig.logicalDevice, memory);
}