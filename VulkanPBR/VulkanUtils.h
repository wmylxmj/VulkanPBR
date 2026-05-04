#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

#include <vector>

struct Texture {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView imageView;
	VkImageAspectFlags imageAspectFlag;
	VkFormat format;
	Texture(VkFormat inFormat, VkImageAspectFlags inImageAspectFlag = VK_IMAGE_ASPECT_COLOR_BIT);
	virtual ~Texture();
};

struct FrameBuffer {
	VkFramebuffer frameBuffer;
	std::vector<Texture*> colorAttachments;
	Texture* depthAttachment;
	Texture* resolveBuffer;
	VkSampleCountFlagBits sampleCount;
	FrameBuffer();
	~FrameBuffer();
};

struct UniformInputsBindings {
	static std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
	static std::vector<VkDescriptorPoolSize> descriptorPoolSizes;
	static VkDescriptorSetLayout descriptorSetLayout;
	static int descriptorSetLayoutCount;
	static void Init();
	static void InitUniformInput(int inBindingPoint, VkShaderStageFlags inVkShaderStageFlags, VkDescriptorType inVkDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
};

struct PipelineStateObject {
public:
	static VkPipelineLayout pipelineLayout;
	static void Init();

	VkRenderPass renderPass;
	VkSampleCountFlagBits sampleCount;
	VkViewport viewport;
	VkRect2D scissor;
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
	VkPipelineColorBlendStateCreateInfo colorBlendState;
	float depthConstantFactor;
	float depthClamp;
	float depthSlopeFactor;
	VkPipeline pipeline;
public:
	PipelineStateObject();
	~PipelineStateObject();
	void CleanUp();
};

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
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
	char** enabledLayers;
	int enabledLayerCount;
	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkRenderPass systemRenderPass;
	FrameBuffer* systemFrameBuffers;
	uint32_t systemFrameBufferCount;
	VkCommandPool commandPool;
	VkSemaphore readyToRenderSemaphore;
	VkSemaphore readyToPresentSemaphore;
};

void GenImage(Texture* texture, uint32_t width, uint32_t height, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount, int mipmaplevel = 1);
VkImageView GenImageView2D(VkImage inImage, VkFormat inFormat, VkImageAspectFlags inImageAspectFlags, int mipmap_level = 1);

GlobalConfig& GetGlobalConfig();
bool InitVulkan(void* param, int width, int height);
