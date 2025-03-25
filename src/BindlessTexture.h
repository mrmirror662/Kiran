#pragma once
#include <Glad/glad.h>
#include<stdint.h>
#include<vector>
#include<stdexcept>
class BindlessTexture
{
private:
	uint64_t textureHandle;
	uint32_t textureId;
	int width;
	int height;
	int channel;

	size_t size;
	std::vector<uint8_t> buffer;
public:
	BindlessTexture(int width, int height, int channel, std::vector<uint8_t> buffer);

	void MakeResident();

	void MakeNonResident();

	uint64_t GetHandle();
	uint32_t GetId();
	bool IsResident();
};
