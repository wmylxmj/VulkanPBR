#include "Scene.h"

#include "VulkanUtils.h"
#include "FrameBuffer.h"

FrameBufferEx* g_HdrFrameBuffer = nullptr;

void OnViewportChanged(int inWidth, int inHeight)
{
	OnViewportChangedVulkan(inWidth, inHeight);
}

void InitScene()
{
	GlobalConfig& globalConfig = GetGlobalConfig();
	g_HdrFrameBuffer = new FrameBufferEx();
	g_HdrFrameBuffer->SetSize(globalConfig.viewportWidth, globalConfig.viewportHeight);
	g_HdrFrameBuffer->AttachColorBuffer(VK_FORMAT_R32G32B32A32_SFLOAT);
	g_HdrFrameBuffer->AttachDepthBuffer();
	g_HdrFrameBuffer->Finish(); // Shader Resource View
}

void RenderOneFrame()
{
	// Render Scene to HDR FrameBuffer
	VkCommandBuffer commandBuffer = g_HdrFrameBuffer->BeginRendering(); // HDR Render Pass

	vkCmdEndRenderPass(commandBuffer);
	// Render HDR to Swapchain Image
	BeginRendering(commandBuffer); // Swapchain Render Pass
	EndRendering();
	SwapBuffers();
}