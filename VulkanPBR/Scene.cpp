#include "Scene.h"

#include "VulkanUtils.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "Node.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

glm::mat4 g_projectionMatrix;
Camera g_mainCamera;

FrameBufferEx* g_HdrFrameBuffer = nullptr;

Node* g_sphereNode = nullptr;

void OnViewportChanged(int inWidth, int inHeight)
{
	g_projectionMatrix = glm::perspective(3.1415926f / 4.0f, (float)inWidth / (float)inHeight, 0.1f, 1000.0f);
	OnViewportChangedVulkan(inWidth, inHeight);
}

void InitScene()
{
	g_mainCamera.m_viewMatrix = glm::lookAt(
		glm::vec3(0.0f, 0.0f, 5.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	GlobalConfig& globalConfig = GetGlobalConfig();
	g_HdrFrameBuffer = new FrameBufferEx();
	g_HdrFrameBuffer->SetSize(globalConfig.viewportWidth, globalConfig.viewportHeight);
	g_HdrFrameBuffer->AttachColorBuffer(VK_FORMAT_R32G32B32A32_SFLOAT);
	g_HdrFrameBuffer->AttachDepthBuffer();
	g_HdrFrameBuffer->Finish(); // Shader Resource View

	g_sphereNode = new Node();
	g_sphereNode->m_modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	Material* material = new Material("Resources/PBR.vsb", "Resources/PBR.fsb");
	g_sphereNode->m_material = material;
	StaticMeshComponent* sphereMesh = new StaticMeshComponent();
	sphereMesh->LoadFromFile("Resources/Model/Sphere.staticmesh");
	g_sphereNode->m_staticMeshComponent = sphereMesh;
	material->m_pipelineStateObject->viewport = {
		0.0f, (float)globalConfig.viewportHeight,
		(float)globalConfig.viewportWidth, -(float)globalConfig.viewportHeight,
		0.0f, 1.0f
	};
	material->m_pipelineStateObject->scissor = {
		{0, 0}, {globalConfig.viewportWidth, globalConfig.viewportHeight}
	};
	SetColorAttachmentCount(material->m_pipelineStateObject, 1);
	material->m_pipelineStateObject->renderPass = g_HdrFrameBuffer->m_renderPass;
	material->m_pipelineStateObject->sampleCount = VK_SAMPLE_COUNT_1_BIT;
}

void RenderOneFrame()
{
	// Render Scene to HDR FrameBuffer
	VkCommandBuffer commandBuffer = g_HdrFrameBuffer->BeginRendering(); // HDR Render Pass
	g_sphereNode->Draw(commandBuffer, g_projectionMatrix, g_mainCamera);

	vkCmdEndRenderPass(commandBuffer);
	// Render HDR to Swapchain Image
	BeginRendering(commandBuffer); // Swapchain Render Pass
	EndRendering();
	SwapBuffers();
}