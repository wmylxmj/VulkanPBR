#pragma once

#include "VulkanUtils.h"

class Material
{
public:
	Material(const char* inVSPath, const char* inFSPath);
	void SetFrontFace(VkFrontFace inVkFrontFace);
	void EnableDepthTest(bool inEnable);
	void SetTexture(int inBindingPoint, VkImageView inVkImageView, VkSampler inVkSampler = nullptr);
	void SetUBO(int inBindingPosition, BufferObject* inUniformBufferObject);
	void SetVec4(int inIndex, float* v);
	void SetVec4(int inIndex, float x, float y, float z, float w = 0.0f);
	void SetCameraWorldPosition(float inX, float inY, float inZ, float inW = 1.0f);
	void Active(VkCommandBuffer inCommandBuffer, int inVertexDataSize);

	static VkSampler sm_defaultSampler;
	GPUProgram* m_gpuProgram;
	PipelineStateObject* m_pipelineStateObject;
	VkPrimitiveTopology m_primitiveTopology;
	std::vector<VkWriteDescriptorSet> m_writeDescriptor;
	VkDescriptorSet m_descriptorSet;
	VkDescriptorPool m_descriptorPool;
	Vec4UniformBufferData* m_vec4s;
	bool m_bEnableDepthTest;
	bool m_bEnableDepthWrite;
	VkFrontFace m_frontFace;
	bool m_bNeedUpdatePSO;
};
