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
vec3 get_emission(Material m)
{
	return m.emission_color * m.emission_power;
}
struct Sphere
{
	alignas(16)vec3 center;
	float r;
	alignas(16)Material mat;
};
