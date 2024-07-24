#pragma once

#include<string>
#include <glm/glm.hpp>
#include"primitives.h"
#include <OBJ_Loader/OBJ_Loader.h>

glm::vec3 objtoglm(const objl::Vector3& in);

std::vector<Triangle> loadFromObj(const std::string meshPath);
