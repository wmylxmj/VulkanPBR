#include "Utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

float GetFrameTime()
{
	static unsigned long lastTime = 0, timeSinceComputerStart = 0;
	timeSinceComputerStart = timeGetTime();
	unsigned long frameTime = lastTime == 0 ? 0 : timeSinceComputerStart - lastTime;
	lastTime = timeSinceComputerStart;
	return float(frameTime) / 1000.0f;
}

unsigned char* LoadFileContent(const char* path, int& fileSize)
{
	FILE* pFile = nullptr;
	errno_t err = fopen_s(&pFile, path, "rb");
	if (err == 0)
	{
		fseek(pFile, 0, SEEK_END);
		long fileSize = ftell(pFile);
		rewind(pFile);
		unsigned char* fileContent = new unsigned char[fileSize];
		fread(fileContent, 1, fileSize, pFile);
		fclose(pFile);
		return fileContent;
	}
	return nullptr;
}

unsigned char* LoadImageFromFile(const char* path, int& width, int& height, int& channel, int forceChannel)
{
	unsigned char* result = stbi_load(path, &width, &height, &channel, forceChannel);
	if (result == nullptr) {
		return nullptr;
	}
	return result;
}