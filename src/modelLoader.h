#pragma once

#include<string>
#include <glm/glm.hpp>
#include"primitives.h"
#include <tuple>


std::tuple<std::vector<Triangle>, std::vector<Material>, std::vector<Image>>
loadFromObjWithMaterials(const std::string& meshPath);


// Appends the glTF's textures, materials and meshes to `scene`, and one instance per
// mesh node (EXT_mesh_gpu_instancing expands to one instance per entry).
bool loadGLTF(const std::string& path, Scene& scene, const glm::mat4& rootTransform = glm::mat4(1.0f));