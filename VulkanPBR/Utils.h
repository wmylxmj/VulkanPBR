#pragma once

extern "C" {
#include "stb_image.h"
}

#include <Windows.h>
#pragma comment(lib,"winmm.lib")

struct StaticMeshVertexData {
	float position[4];
	float texcoord[4];
	float normal[4];
};
struct StaticMeshVertexDataEx :public StaticMeshVertexData {
	float tangent[4];
};

struct SubMesh {
	unsigned int* indexes;
	int indexCount;
};

float GetFrameTime();
unsigned char* LoadFileContent(const char* path, int& fileSize);
unsigned char* LoadImageFromFile(const char* path, int& width, int& height, int& channel, int forceChannel);
