#pragma once

#include "VulkanUtils.h"

class FrameBufferEx
{
public:
	FrameBufferEx();
	~FrameBufferEx();
	void SetSize(uint32_t width, uint32_t height);
	void AttachColorBuffer(VkFormat inFormat = VK_FORMAT_R8G8B8A8_UNORM);
	void AttachDepthBuffer();
	void Finish(VkImageLayout inColorBufferFinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	void SetClearColor(int index, float r, float g, float b, float a);
	void SetClearDepthStencil(float depth, uint32_t stencil);
	VkCommandBuffer BeginRendering(VkCommandBuffer inCommandBuffer = nullptr);

public:
	VkFramebuffer m_fbo;
	VkRenderPass m_renderPass;
	uint32_t m_width;
	uint32_t m_height;
	std::vector<Texture*> m_attachments;
	std::vector<VkClearValue> m_clearValues;
	int m_colorBufferCount;
	int m_depthBufferIndex;
	Texture* m_depthBuffer;
};
