#pragma once

#include "primitives.h"
#include "modelLoader.h"
#include "imageLoader.h"
#include <iostream>
inline std::vector<Triangle> makeCube(const glm::vec3& center, float size, uint32_t matIndex) {
	using namespace glm;

	std::vector<Triangle> cubeTriangles;

	const float h = size * 0.5f;

	// 8 cube vertices
	vec3 p[8] = {
		center + vec3(-h, -h, -h),
		center + vec3(h, -h, -h),
		center + vec3(h,  h, -h),
		center + vec3(-h,  h, -h),
		center + vec3(-h, -h,  h),
		center + vec3(h, -h,  h),
		center + vec3(h,  h,  h),
		center + vec3(-h,  h,  h)
	};

	// Basic UVs (square tiling)
	vec2 uv0(0.0f, 0.0f);
	vec2 uv1(1.0f, 0.0f);
	vec2 uv2(1.0f, 1.0f);
	vec2 uv3(0.0f, 1.0f);

	auto makeTri = [&](const vec3& a, const vec3& b, const vec3& c,
		const vec2& uva, const vec2& uvb, const vec2& uvc) {
			vec3 n = normalize(cross(b - a, c - a));
			return Triangle{
				a, b, c,
				static_cast<int>(matIndex),
				n, n, n,
				uva, uvb, uvc,
				true, false, static_cast<uint32_t>(-1)
			};
		};

	auto pushQuad = [&](int a, int b, int c, int d) {
		// Tri 1: a, b, c
		cubeTriangles.push_back(makeTri(p[a], p[b], p[c], uv0, uv1, uv2));
		// Tri 2: a, c, d
		cubeTriangles.push_back(makeTri(p[a], p[c], p[d], uv0, uv2, uv3));
		};

	// Faces
	pushQuad(0, 1, 2, 3); // -Z (back)
	pushQuad(5, 4, 7, 6); // +Z (front)
	pushQuad(4, 0, 3, 7); // -X (left)
	pushQuad(1, 5, 6, 2); // +X (right)
	pushQuad(3, 2, 6, 7); // +Y (top)
	pushQuad(4, 5, 1, 0); // -Y (bottom)

	return cubeTriangles;
}


inline Scene testSceneDragon()
{
	using namespace glm;

	std::vector<Triangle> dragonTriangles;
	std::vector<Material> dragonMats;
	std::vector<Image> dragonTextures;
	std::tie(dragonTriangles, dragonMats, dragonTextures) = loadFromObjWithMaterials("assets/salle_de_bain.obj");


	HDRI env = imgutl::loadHDRI("assets/monkstown_castle_4k.exr");

	std::vector<Triangle> sceneTriangles;
	std::vector<Material> materials;

	sceneTriangles.insert(sceneTriangles.end(), dragonTriangles.begin(), dragonTriangles.end()); // Add the dragon
	materials.insert(materials.end(), dragonMats.begin(), dragonMats.end());
	std::vector<Image> colorMaps;
	colorMaps.insert(colorMaps.end(), dragonTextures.begin(), dragonTextures.end());

	Scene scene;
	scene.triangles = sceneTriangles;
	// scene.spheres = scatteredSpheres;
	scene.mats = materials;
	scene.colorMaps = colorMaps;
	scene.hdr = env;

	// Return the Scene
	return scene;
}

inline Scene testSceneGLTF() {
	using namespace glm;

	std::vector<Triangle> triangles;
	std::vector<Material> materials;
	std::vector<Image> textures;

	HDRI env = imgutl::loadHDRI("assets/sky2.exr");
	std::tie(triangles, materials, textures) = loadFromGLTFWithMaterials("assets/glTF/Sponza.gltf");

	Scene scene;
	scene.triangles = std::move(triangles);
	scene.mats = std::move(materials);
	scene.colorMaps = std::move(textures);
	scene.hdr = env;

	// Cube parameters
	const float cubeSize = 64.0f;
	const vec3 startPos = vec3(-221.51f, 700.05f, -4.96f);
	const vec3 spacing = vec3(260.0f, 0.0f, 0.0f);

	// Color palette
	std::vector<vec3> colors = {
		// Warm white / soft yellow (incandescent)
		vec3(1.0f, 0.937f, 0.835f),  // warm white

		// Natural daylight white
		vec3(0.960f, 0.960f, 1.0f),  // cool daylight

		// Soft orange-tungsten
		vec3(1.0f, 0.694f, 0.388f),  // soft orange

		// Pale sky blue
		vec3(0.678f, 0.847f, 0.902f), // sky blue

		// Candlelight amber
		vec3(1.0f, 0.843f, 0.666f)   // amber
	};
	// Generate 5 cubes with different colors
	for (int i = 0; i < 5; ++i) {
		Material cubeMat;
		cubeMat.albedo = colors[i];
		cubeMat.roughness = 0.5f;
		cubeMat.metallic = 0.0f;
		cubeMat.emission_power = 15;
		cubeMat.emission_color = colors[i];

		uint32_t matIndex = static_cast<uint32_t>(scene.mats.size());
		scene.mats.push_back(cubeMat);

		vec3 center = startPos + spacing * float(i);
		auto cube = makeCube(center, cubeSize, matIndex);
		scene.triangles.insert(scene.triangles.end(), cube.begin(), cube.end());
	}

	return scene;
}
