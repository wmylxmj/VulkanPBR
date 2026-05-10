#include "Node.h"

#include "Camera.h"
#include "glm/ext.hpp"

Node::Node()
{
	m_staticMeshComponent = nullptr;
	m_mat4UBOData = new Mat4UniformBufferData(1024);
}

void Node::Draw(VkCommandBuffer inCommandBuffer, glm::mat4& inProjectionMatrix, Camera& inCamera)
{
	m_mat4UBOData->SetMat4(0, glm::value_ptr(m_modelMatrix));
	m_mat4UBOData->SetMat4(1, glm::value_ptr(inCamera.m_viewMatrix));
	m_mat4UBOData->SetMat4(2, glm::value_ptr(inProjectionMatrix));
	glm::mat4 itModelMatrix = glm::inverseTranspose(m_modelMatrix);
	m_mat4UBOData->SetMat4(3, glm::value_ptr(itModelMatrix));

	if (m_mat4UBOData->ubo == nullptr) {
		m_mat4UBOData->UpdateGPUData();
		m_material->SetUBO(0, m_mat4UBOData->ubo);
	}
	else {
		m_mat4UBOData->UpdateGPUData();
	}
	m_staticMeshComponent->Update(inCommandBuffer);
	m_material->m_vec4s->SetVec4(0, glm::value_ptr(inCamera.m_position));
	m_material->Active(inCommandBuffer, m_staticMeshComponent->m_vertexDataSize);
	SetDynamicState(m_material->m_pipelineStateObject, inCommandBuffer);
	m_staticMeshComponent->Draw(inCommandBuffer);
}