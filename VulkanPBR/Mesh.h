#pragma once

#include "VulkanUtils.h"
#include "Utils.h"
#include "Material.h"

#include <unordered_map>
#include <string>

struct VulkanSubMesh : public SubMesh {
	BufferObject* ibo;
	VulkanSubMesh() {
		ibo = nullptr;
	}
};

class StaticMeshComponent {
public:
	StaticMeshVertexData* m_vertexData;
	int m_vertexCount;
	int m_vertexDataSize;
	BufferObject* m_vbo;
	std::unordered_map<std::string, VulkanSubMesh*> m_subMeshes;
	void SetPosition(int index, float x, float y, float z, float w = 1.0f);
	void SetTexcoord(int index, float x, float y, float z = 1.0f, float w = 1.0f);
	void SetNormal(int index, float x, float y, float z, float w = 0.0f);
	void LoadFromFile(const char* inFilePath);
	void LoadFromFile2(const char* inFilePath);
	StaticMeshComponent();
	virtual void Update(VkCommandBuffer inCommandBuffer);
	virtual void Draw(VkCommandBuffer inCommandBuffer);
};
