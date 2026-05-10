#pragma once

#include "VulkanUtils.h"
#include "Mesh.h"

#include "glm/glm.hpp"

class Camera;

class Node
{
public:
	Node();
	void Draw(VkCommandBuffer inCommandBuffer, glm::mat4& inProjectionMatrix, Camera& inCamera);

	StaticMeshComponent* m_staticMeshComponent;
	glm::mat4 m_modelMatrix;
	Material* m_material;
	Mat4UniformBufferData* m_mat4UBOData;
};
