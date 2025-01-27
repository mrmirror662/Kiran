#pragma once

#include <glm/glm.hpp>
#include "imageLoader.h"
struct Triangle
{
	alignas(16)glm::vec3 v0;
	alignas(16)glm::vec3 v1;
	//not 16
	alignas(16)glm::vec3 v2; // vertices
	alignas(4) int matIndex;
	alignas(16)glm::vec3 n0;
	alignas(16)glm::vec3 n1;
	alignas(16)glm::vec3 n2;
	bool hasNormal = false;
	bool hasTexture = false;
	alignas (8) uint64_t textureHandle = 0;
};

struct Sphere
{
	alignas(16)glm::vec3 center;
	float r;
	//16
	int matIndex;
};

struct Material
{
	alignas(16)glm::vec3 albedo;
	float metallic;
	alignas(16)glm::vec3 emission_color;
	float roughness;
	//16
	float emission_power;
	float eta;
};

struct Scene
{
	std::vector<Triangle> triangles;
	std::vector<Sphere> spheres;
	std::vector<Material> mats;
	HDRI hdr;
};
