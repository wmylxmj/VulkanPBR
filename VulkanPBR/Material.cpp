#include "Material.h"

VkSampler Material::sm_defaultSampler = nullptr;

Material::Material(const char* inVSPath, const char* inFSPath)
{
	m_gpuProgram = new GPUProgram;
	m_gpuProgram->AttachShader(VK_SHADER_STAGE_VERTEX_BIT, inVSPath);
	m_gpuProgram->AttachShader(VK_SHADER_STAGE_FRAGMENT_BIT, inFSPath);
	if (sm_defaultSampler == nullptr) {
		sm_defaultSampler = GenSampler(
			VK_FILTER_LINEAR, VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
		);
	}
	m_bEnableDepthTest = true;
	m_bEnableDepthWrite = true;
	m_frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	m_pipelineStateObject = new PipelineStateObject;
	m_bNeedUpdatePSO = true;
	m_vec4s = new Vec4UniformBufferData(4096);

	m_descriptorPool = InitDescriptorPool();
	m_descriptorSet = InitDescriptorSet(m_descriptorPool);

	m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

void Material::SetFrontFace(VkFrontFace inVkFrontFace)
{
	m_frontFace = inVkFrontFace;
	m_bNeedUpdatePSO = true;
}

void Material::EnableDepthTest(bool inEnable)
{
	m_bEnableDepthTest = inEnable;
	m_bNeedUpdatePSO = true;
}

void Material::SetTexture(int inBindingPoint, VkImageView inVkImageView, VkSampler inVkSampler)
{
	if (inVkSampler == nullptr) {
		inVkSampler = sm_defaultSampler;
	}
	VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo;
	imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo->imageView = inVkImageView;
	imageInfo->sampler = inVkSampler;

	VkWriteDescriptorSet descriptorWriter = {};
	descriptorWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWriter.dstSet = m_descriptorSet;
	descriptorWriter.dstBinding = inBindingPoint;
	descriptorWriter.dstArrayElement = 0;
	descriptorWriter.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWriter.descriptorCount = 1;
	descriptorWriter.pImageInfo = imageInfo;
	m_writeDescriptor.push_back(descriptorWriter);
}

void Material::SetUBO(int inBindingPosition, BufferObject* inUniformBufferObject)
{
	VkDescriptorBufferInfo* bufferInfo = new VkDescriptorBufferInfo;
	bufferInfo->buffer = inUniformBufferObject->buffer;
	bufferInfo->offset = 0;
	bufferInfo->range = inUniformBufferObject->size;

	VkWriteDescriptorSet descriptorWriter = {};
	descriptorWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWriter.dstSet = m_descriptorSet;
	descriptorWriter.dstBinding = inBindingPosition;
	descriptorWriter.dstArrayElement = 0;
	descriptorWriter.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWriter.descriptorCount = 1;
	descriptorWriter.pBufferInfo = bufferInfo;
	m_writeDescriptor.push_back(descriptorWriter);
}

void Material::SetVec4(int inIndex, float* v)
{
	m_vec4s->SetVec4(inIndex, v);
}

void Material::SetVec4(int inIndex, float x, float y, float z, float w)
{
	float v[4] = { x, y, z, w };
	SetVec4(inIndex, v);
}

void Material::SetCameraWorldPosition(float inX, float inY, float inZ, float inW)
{
	SetVec4(0, inX, inY, inZ, inW);
}

void Material::Active(VkCommandBuffer inCommandBuffer, int inVertexDataSize)
{
	if (m_vec4s->ubo == nullptr) {
		m_vec4s->UpdateGPUData();
		SetUBO(3, m_vec4s->ubo);
	}
	else {
		m_vec4s->UpdateGPUData();
	}
	if (m_bNeedUpdatePSO) {
		m_bNeedUpdatePSO = false;
		CreateGraphicPipeline(
			m_pipelineStateObject,
			inVertexDataSize,
			m_gpuProgram,
			m_bEnableDepthTest,
			m_bEnableDepthWrite,
			m_frontFace,
			m_primitiveTopology
		);
	}
	for (int i = 0; i < m_writeDescriptor.size(); ++i) {
		m_writeDescriptor[i].dstSet = m_descriptorSet;
	}
	vkUpdateDescriptorSets(
		GetGlobalConfig().logicalDevice,
		static_cast<uint32_t>(m_writeDescriptor.size()),
		m_writeDescriptor.data(),
		0,
		nullptr
	);

	vkCmdBindPipeline(inCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineStateObject->pipeline);
	vkCmdBindDescriptorSets(
		inCommandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_pipelineStateObject->pipelineLayout,
		0, 1, &m_descriptorSet, 0, nullptr
	);
}