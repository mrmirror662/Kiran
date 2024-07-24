#pragma once

#include<string>
#include <glm/glm.hpp>
#include"primitives.h"
namespace objl{
    struct Vector3;
}
glm::vec3 objtoglm(const objl::Vector3& in);

std::vector<Triangle> loadFromObj(const std::string meshPath);
