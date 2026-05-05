#pragma once

extern "C" {
#include "stb_image.h"
}

struct StaticMeshVertexData {
	float position[4];
	float texcoord[4];
	float normal[4];
};
struct StaticMeshVertexDataEx :public StaticMeshVertexData {
	float tangent[4];
};

unsigned char* LoadFileContent(const char* path, int& fileSize);
unsigned char* LoadImageFromFile(const char* path, int& width, int& height, int& channel, int forceChannel);
