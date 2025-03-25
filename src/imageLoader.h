#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <string>

struct HDRI
{
	int width;
	int height;
	std::vector<glm::vec4> data;
};

struct Image
{
	std::vector<uint8_t> buffer;
	int channel;
	int width;
	int height;
};
HDRI loadHDRI(const std::string& path);
Image LoadImage(const std::string& path);