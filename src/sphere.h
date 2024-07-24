#pragma once

#include <glm/glm.hpp>
using namespace glm;

struct Material
{
	alignas(16)vec3 albedo;
	float roughness;
	float metallic;
	alignas(16)vec3 emission_color;
	float emission_power;
	float eta;
};

struct Sphere
{
	alignas(16)vec3 center;
	float r;
	alignas(16)Material mat;
};
