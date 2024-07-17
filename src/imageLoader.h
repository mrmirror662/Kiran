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


HDRI loadHDRI(const std::string& path);
