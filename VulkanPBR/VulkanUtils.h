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

struct BufferObject {
	VkBuffer buffer;
	VkDeviceMemory memory;
	int size;
	BufferObject() {
		buffer = nullptr;
		memory = nullptr;
		size = 0;
	}
	void Write(void* inData, int inDataSize = 0);
};

struct Mat4UniformBufferData {
	int mat4Count;
	float data[16384]; // 1024 * 16
	BufferObject* ubo;
	bool bNeedSyncData;
	Mat4UniformBufferData(int inMat4Count);
	void SetMat4(int inIndex, float* inMat4);
	void UpdateGPUData();
};

struct Vec4UniformBufferData {
	int vec4Count;
	float data[4096]; // 1024 * 4
	BufferObject* ubo;
	bool bNeedSyncData;
	Vec4UniformBufferData(int inVec4Count);
	void SetVec4(int inIndex, float* inVec4);
	void UpdateGPUData();
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
	VkCommandBuffer systemRenderPassCommandBuffer;
};

void GenImage(Texture* texture, uint32_t width, uint32_t height, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount, int mipmaplevel = 1);
VkImageView GenImageView2D(VkImage inImage, VkFormat inFormat, VkImageAspectFlags inImageAspectFlags, int mipmap_level = 1);
void SubmitImage2D(Texture* texture, int width, int height, const void* pixel);
VkSampler GenSampler(VkFilter inMinFilter, VkFilter inMagFilter, VkSamplerAddressMode inSamplerAddressModeU, VkSamplerAddressMode inSamplerAddressModeV, VkSamplerAddressMode inSamplerAddressModeW);
VkSampler GenCubeMapSampler(VkFilter inMinFilter = VK_FILTER_LINEAR, VkFilter inMagFilter = VK_FILTER_LINEAR,
	VkSamplerAddressMode inSamplerAddressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	VkSamplerAddressMode inSamplerAddressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	VkSamplerAddressMode inSamplerAddressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
void GenImageCube(Texture* texture, uint32_t w, uint32_t h, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount, int mipmapLevel = 1);
VkImageView GenImageViewCube(VkImage inImage, VkFormat inFormat, VkImageAspectFlags inImageAspectFlags, int mipmapLevel = 1);

void TransferImageLayout(VkCommandBuffer inCommandBuffer, VkImage inImage, VkImageSubresourceRange inSubresourceRange,
	VkImageLayout inOldLayout, VkAccessFlags inOldAccessFlags, VkPipelineStageFlags inSrcStageMask,
	VkImageLayout inNewLayout, VkAccessFlags inNewAccessFlags, VkPipelineStageFlags inDstStageMask);
GlobalConfig& GetGlobalConfig();
bool InitVulkan(void* param, int width, int height);

VkResult GenCommandBuffer(VkCommandBuffer* commandBuffer, int count, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
void DeleteCommandBuffer(VkCommandBuffer* commandBuffer, int count);
VkCommandBuffer BeginRendering(VkCommandBuffer inCommandBuffer);
void EndRendering();
void SwapBuffers();
VkResult BeginOneTimeCommandBuffer(VkCommandBuffer* commandBuffer);
void WaitForCommmandFinish(VkCommandBuffer commandBuffer);
VkResult EndOneTimeCommandBuffer(VkCommandBuffer commandBuffer);

VkResult GenBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
BufferObject* CreateBuffer(VkDeviceSize inVkDeviceSize, VkBufferUsageFlags inVkBufferUsageFlags, VkMemoryPropertyFlags inVkMemoryPropertyFlags);
void BufferSubData(VkBuffer buffer, VkBufferUsageFlags usage, const  void* data, VkDeviceSize size);

void MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData);
void UnmapMemory(VkDeviceMemory memory);
