#include "FrameBuffer.h"

FrameBufferEx::FrameBufferEx()
{
	m_fbo = nullptr;
	m_renderPass = nullptr;
	m_depthBuffer = nullptr;
}

FrameBufferEx::~FrameBufferEx()
{
	for (auto iter = m_attachments.begin(); iter != m_attachments.end(); ++iter) {
		delete* iter;
	}
	GlobalConfig& globalConfig = GetGlobalConfig();
	if (m_renderPass != nullptr) {
		vkDestroyRenderPass(globalConfig.logicalDevice, m_renderPass, nullptr);
	}
	if (m_fbo != nullptr) {
		vkDestroyFramebuffer(globalConfig.logicalDevice, m_fbo, nullptr);
	}
}

void FrameBufferEx::SetSize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}

void FrameBufferEx::AttachColorBuffer(VkFormat inFormat)
{
	Texture* colorBuffer = new Texture(inFormat);
	colorBuffer->format = inFormat;
	GenImage(
		colorBuffer,
		m_width,
		m_height,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_SAMPLE_COUNT_1_BIT
	);
	colorBuffer->imageView = GenImageView2D(colorBuffer->image, colorBuffer->format, colorBuffer->imageAspectFlag);
	m_colorBufferCount++;
	m_attachments.push_back(colorBuffer);
	VkClearValue clearValue;
	clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_clearValues.push_back(clearValue);
}

void FrameBufferEx::AttachDepthBuffer()
{
	Texture* depthBuffer = new Texture(VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT);
	depthBuffer->format = VK_FORMAT_D24_UNORM_S8_UINT;
	GenImage(
		depthBuffer,
		m_width,
		m_height,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_SAMPLE_COUNT_1_BIT
	);
	depthBuffer->imageView = GenImageView2D(depthBuffer->image, depthBuffer->format, depthBuffer->imageAspectFlag);
	m_depthBufferIndex = m_attachments.size();
	m_attachments.push_back(depthBuffer);
	VkClearValue clearValue;
	clearValue.depthStencil = { 1.0f,0 };
	m_clearValues.push_back(clearValue);
	m_depthBuffer = depthBuffer;
}

void FrameBufferEx::Finish(VkImageLayout inColorBufferFinalLayout)
{
	std::vector<VkAttachmentDescription> attachmentDescriptions;
	std::vector<VkAttachmentReference> colorAttachmentRefences;
	VkAttachmentReference depthAttachmentReference = {};
	std::vector<VkImageView> renderTargets;
	renderTargets.resize(m_attachments.size());
	attachmentDescriptions.resize(m_attachments.size());
	int color_buffer_count = 0;
	for (size_t i = 0; i < m_attachments.size(); ++i) {
		Texture* texture = m_attachments[i];
		if (texture->imageAspectFlag == VK_IMAGE_ASPECT_COLOR_BIT) {
			color_buffer_count++;
		}
		else if (texture->imageAspectFlag == VK_IMAGE_ASPECT_DEPTH_BIT) {
		}
		renderTargets[i] = texture->imageView;
	}
	colorAttachmentRefences.resize(color_buffer_count);
	int colorBufferIndex = 0;
	int attachmentPoint = 0;
	for (size_t i = 0; i < m_attachments.size(); ++i) {
		Texture* texture = m_attachments[i];
		if (texture->imageAspectFlag == VK_IMAGE_ASPECT_COLOR_BIT) {
			attachmentDescriptions[i] = {
				0,
				texture->format,
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_STORE,
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				inColorBufferFinalLayout
			};
			colorAttachmentRefences[colorBufferIndex++] = {
				uint32_t(attachmentPoint++),VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};
		}
		else if (texture->imageAspectFlag == VK_IMAGE_ASPECT_DEPTH_BIT) {
			attachmentDescriptions[i] = {
				0,
				texture->format,
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_STORE,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
			depthAttachmentReference.attachment = attachmentPoint++;
			depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
	}

	VkSubpassDescription subpasses = {};
	subpasses.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses.colorAttachmentCount = colorAttachmentRefences.size();
	subpasses.pColorAttachments = colorAttachmentRefences.data();
	subpasses.pDepthStencilAttachment = &depthAttachmentReference;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = attachmentDescriptions.size();
	renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpasses;
	GlobalConfig& globalConfig = GetGlobalConfig();
	vkCreateRenderPass(globalConfig.logicalDevice, &renderPassCreateInfo, nullptr, &m_renderPass);

	VkFramebufferCreateInfo framebufferCreateInfo = {};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.pAttachments = renderTargets.data();
	framebufferCreateInfo.attachmentCount = renderTargets.size();
	framebufferCreateInfo.width = m_width;
	framebufferCreateInfo.height = m_height;
	framebufferCreateInfo.layers = 1;
	framebufferCreateInfo.renderPass = m_renderPass;
	vkCreateFramebuffer(globalConfig.logicalDevice, &framebufferCreateInfo, nullptr, &m_fbo);
}

void FrameBufferEx::SetClearColor(int index, float r, float g, float b, float a)
{
	m_clearValues[index].color = { r,g,b,a };
}

void FrameBufferEx::SetClearDepthStencil(float depth, uint32_t stencil)
{
	m_clearValues[m_depthBufferIndex].depthStencil = { depth,stencil };
}

VkCommandBuffer FrameBufferEx::BeginRendering(VkCommandBuffer inCommandBuffer)
{
	VkCommandBuffer commandBuffer = nullptr;
	if (inCommandBuffer != nullptr) {
		commandBuffer = inCommandBuffer;
	}
	else {
		BeginOneTimeCommandBuffer(&commandBuffer);
	}
	VkFramebuffer renderTarget = m_fbo;
	VkRenderPass renderPass = m_renderPass;

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.framebuffer = renderTarget;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = { m_width, m_height };
	renderPassBeginInfo.clearValueCount = m_clearValues.size();
	renderPassBeginInfo.pClearValues = m_clearValues.data();
	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	return commandBuffer;
}