#pragma once

#include<string>
#include <glm/glm.hpp>
#include"primitives.h"
#include <tuple>


std::tuple<std::vector<Triangle>, std::vector<Material>, std::vector<Image>>
loadFromObjWithMaterials(const std::string& meshPath);


std::tuple<std::vector<Triangle>, std::vector<Material>, std::vector<Image>>
loadFromGLTFWithMaterials(const std::string& meshPath);