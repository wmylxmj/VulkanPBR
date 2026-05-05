#include "Utils.h"

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