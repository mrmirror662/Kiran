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
	alignas(8)glm::vec2 uv0;
	alignas(8)glm::vec2 uv1;
	alignas(8)glm::vec2 uv2;
	alignas(4)uint32_t hasNormal = false;
	alignas(4)uint32_t hasTexture = false;
	alignas(4)uint32_t colorMapIndex = -1;


	static void AssignMaterialIndexForMesh(std::vector<Triangle>& mesh, uint32_t index)
	{
		for (auto& t : mesh)
		{
			t.matIndex = index;
		}
	}
	static void AssignColorIndexForMesh(std::vector<Triangle>& mesh, uint32_t index)
	{
		for (auto& t : mesh)
		{
			t.colorMapIndex = index;
		}
	}
	static void ActivateTextureForMesh(std::vector<Triangle>& mesh)
	{
		for (auto& t : mesh)
		{
			t.hasTexture = true;
		}
	}
	static void DeactivateTextureForMesh(std::vector<Triangle>& mesh)
	{
		for (auto& t : mesh)
		{
			t.hasTexture = false;
		}
	}
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
	std::vector<Image> colorMaps;
	HDRI hdr;
};
