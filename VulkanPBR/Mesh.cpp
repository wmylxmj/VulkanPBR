#include "Mesh.h"

#include <format>

void StaticMeshComponent::SetPosition(int index, float x, float y, float z, float w)
{
	m_vertexData[index].position[0] = x;
	m_vertexData[index].position[1] = y;
	m_vertexData[index].position[2] = z;
	m_vertexData[index].position[3] = w;
}

void StaticMeshComponent::SetTexcoord(int index, float x, float y, float z, float w)
{
	m_vertexData[index].texcoord[0] = x;
	m_vertexData[index].texcoord[1] = y;
	m_vertexData[index].texcoord[2] = z;
	m_vertexData[index].texcoord[3] = w;
}

void StaticMeshComponent::SetNormal(int index, float x, float y, float z, float w)
{
	m_vertexData[index].normal[0] = x;
	m_vertexData[index].normal[1] = y;
	m_vertexData[index].normal[2] = z;
	m_vertexData[index].normal[3] = w;
}

void StaticMeshComponent::LoadFromFile(const char* inFilePath)
{
	FILE* pFile = nullptr;
	errno_t err = fopen_s(&pFile, inFilePath, "rb");
	if (err == 0) {
		int temp = 0;
		fread(&temp, sizeof(int), 1, pFile);
		m_vertexCount = temp;
		m_vertexData = new StaticMeshVertexData[m_vertexCount];
		m_vertexDataSize = sizeof(StaticMeshVertexData);
		fread(m_vertexData, sizeof(StaticMeshVertexData), m_vertexCount, pFile);
		while (!feof(pFile)) {
			int nameLen = 0, indexCount = 0;
			fread(&nameLen, 1, sizeof(int), pFile);
			if (feof(pFile)) {
				break;
			}
			char name[256] = { 0 };
			fread(name, 1, sizeof(char) * nameLen, pFile);
			fread(&indexCount, 1, sizeof(int), pFile);
			VulkanSubMesh* submesh = new VulkanSubMesh;
			submesh->indexCount = indexCount;
			submesh->indexes = new unsigned int[indexCount];
			fread(submesh->indexes, 1, sizeof(unsigned int) * indexCount, pFile);
			m_subMeshes.insert(std::pair<std::string, VulkanSubMesh*>(name, submesh));

			std::string formattedString = std::format("Load StaticMesh [%s] : vertex count[%d] submesh[%s] Index[%d]\n", inFilePath, m_vertexCount, name, indexCount);
			OutputDebugStringA(formattedString.c_str());
		}
		fclose(pFile);
	}
}

void StaticMeshComponent::LoadFromFile2(const char* inFilePath)
{
	FILE* pFile = nullptr;
	errno_t err = fopen_s(&pFile, inFilePath, "rb");
	if (err == 0) {
		int vertice_count;
		fread(&vertice_count, 1, sizeof(int), pFile);
		m_vertexCount = vertice_count;
		m_vertexData = new StaticMeshVertexDataEx[vertice_count];
		m_vertexDataSize = sizeof(StaticMeshVertexDataEx);
		fread(m_vertexData, 1, sizeof(StaticMeshVertexDataEx) * vertice_count, pFile);
		while (!feof(pFile)) {
			int nameLen = 0, indexCount = 0;
			fread(&nameLen, 1, sizeof(int), pFile);
			if (feof(pFile)) {
				break;
			}
			char name[256] = { 0 };
			fread(name, 1, sizeof(char) * nameLen, pFile);
			fread(&indexCount, 1, sizeof(int), pFile);
			VulkanSubMesh* submesh = new VulkanSubMesh;
			submesh->indexCount = indexCount;
			submesh->indexes = new unsigned int[indexCount];
			fread(submesh->indexes, 1, sizeof(unsigned int) * indexCount, pFile);
			m_subMeshes.insert(std::pair<std::string, VulkanSubMesh*>(name, submesh));
			std::string formattedString = std::format("Load StaticMesh [%s] : vertex count[%d] submesh[%s] Index[%d]\n", inFilePath, vertice_count, name, indexCount);
			OutputDebugStringA(formattedString.c_str());
		}
		fclose(pFile);
	}
}

StaticMeshComponent::StaticMeshComponent()
{
	m_vbo = nullptr;
}

void StaticMeshComponent::Update(VkCommandBuffer inCommandBuffer)
{
	if (m_vertexCount == 0) {
		return;
	}
	if (m_vbo == nullptr) {
		m_vbo = CreateBuffer(
			m_vertexDataSize * m_vertexCount,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
		BufferSubData(m_vbo->buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_vertexData, m_vertexDataSize * m_vertexCount);
	}
	if (!m_subMeshes.empty()) {
		std::unordered_map<std::string, VulkanSubMesh*>::iterator iter = m_subMeshes.begin();
		if (iter->second->ibo == nullptr) {
			for (; iter != m_subMeshes.end(); iter++) {
				VulkanSubMesh* submesh = iter->second;
				submesh->ibo = CreateBuffer(
					sizeof(unsigned int) * submesh->indexCount,
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
				);
				BufferSubData(submesh->ibo->buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, submesh->indexes, sizeof(unsigned int) * submesh->indexCount);
			}
		}
	}
}

void StaticMeshComponent::Draw(VkCommandBuffer inCommandBuffer)
{
	VkBuffer vertexBuffers[] = { m_vbo->buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(inCommandBuffer, 0, 1, vertexBuffers, offsets);
	if (m_subMeshes.empty()) {
		vkCmdDraw(inCommandBuffer, m_vertexCount, 1, 0, 0);
	}
	else {
		std::unordered_map<std::string, VulkanSubMesh*>::iterator iter = m_subMeshes.begin();
		for (; iter != m_subMeshes.end(); iter++) {
			vkCmdBindIndexBuffer(inCommandBuffer, iter->second->ibo->buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(inCommandBuffer, iter->second->indexCount, 1, 0, 0, 0);
		}
	}
}

void FullScreenQuadMeshComponent::Init()
{
	m_vertexCount = 4;
	m_vertexData = new StaticMeshVertexData[4];
	m_vertexDataSize = sizeof(StaticMeshVertexData);
	SetPosition(0, -1.0f, -1.0f, 0.0f);
	SetPosition(1, 1.0f, -1.0f, 0.0f);
	SetPosition(2, -1.0f, 1.0f, 0.0f);
	SetPosition(3, 1.0f, 1.0f, 0.0f);
	SetTexcoord(0, 0.0f, 0.0f);
	SetTexcoord(1, 1.0f, 0.0f);
	SetTexcoord(2, 0.0f, 1.0f);
	SetTexcoord(3, 1.0f, 1.0f);
}