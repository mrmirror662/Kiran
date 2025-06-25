#pragma once

#include"primitives.h"
#include "modelLoader.h"
#include "imageLoader.h"
#include <iostream>
inline Scene testSceneBox()
{
	using namespace glm;
	Image checker_board = LoadImage("assets/board.png");
	std::vector<Image> colorMaps;
	colorMaps.push_back(checker_board); // Index 0: checker_board

	// Define materials
	std::vector<Material> materials = {
		Material(vec3(0.99, 0.99, 0.79), 0.7, vec3(0.0f), 0.1, 0.0, 0.0), // 0: Blue (for the wall)
		Material(vec3(0.9, 0.69, 0.69), 0.99, vec3(0.0f), 0.01, 0.0, 0.0), // 1: Yellow (highly reflective, for small cube and wall)
		Material(vec3(0.9, 0.9, 0.5), 0, vec3(0.0f), 0.00001, 0.0, 0.0), // 2: White (floor - will use checkerboard)
		Material(vec3(0.9, 0.9, 0.9), 0.1, vec3(0.0f), 0.5, 0.0, 0.0), // 3: White (ceiling - for the actual room ceiling geometry, not the light)
		Material(vec3(0.9, 0.9, 0.9), 0.6, vec3(0.0f), 0.8, 0.0, 0), // 4: Generic opaque white/grey (for walls)
		Material(vec3(0.9, 0.9, 0.9), 0.9, vec3(0.0f), 0.1, 0.0, 0.0), // 5: Chrome (highly metallic, low roughness sphere)
		Material(vec3(0.99, 0.99, 0.99), 0.0, vec3(0.0f), 0.001, 0.0, 1.1), // 6: Clear Glass (low roughness, high eta)
		Material(vec3(1.0, 0.8, 0.0), 0.0, vec3(1.0f, 0.8f, 0.0f), 0.0, 10.0, 0.0), // 7: Intense Orange Emissive (for point light)
		Material(vec3(0.0, 1.0, 0.0), 0.0, vec3(0.0f, 1.0f, 0.0f), 0.0, 8.0, 0.0), // 8: Intense Green Emissive (for point light)
		Material(vec3(0.9, 0.1, 0.1), 0.0, vec3(0.9f, 0.1f, 0.1f), 0.0, 20.0, 0.0), // 9: Intense Red Emissive (for point light)
		Material(vec3(1.0, 1.0, 1.0), 0.0, vec3(0.6f, 0.6f, 0.6f), 0.0, 0.5, 0.0), // 10: Emissive white (main ceiling light)
		Material(vec3(1.0, 0.0, 0.0), 0.0, vec3(1.0f, 0.0f, 0.0f), 0.0, 55.0 , 0.0) // 11: NEW: Bright Red Emissive (for the new cube - **BRIGHTER!**)
	};

	// Define spheres inside the cube
	std::vector<Sphere> scatteredSpheres = {
		Sphere(vec3(1.5, -3.0, -3.0), 0.7, 6), // Large Glass sphere (Material 6) - Diameter 1.4
		Sphere(vec3(-2.5, -4.2, 3.0), 0.4, 5), // Chrome sphere (Material 5)
		// Super small and bright lights
		Sphere(vec3(-1.0, -1.5, 0.5), 0.05, 7), // Intense Orange Point Light Sphere (Material 7, smaller radius)
		Sphere(vec3(0.0, -1.8, -4.0), 0.05, 8), // Intense Green Point Light Sphere (Material 8, smaller radius)
		Sphere(vec3(2.0, -2.2, 1.0), 0.05, 9) // Intense Red Point Light Sphere (Material 9, smaller radius)
	};

	// Cube vertices (for the room)
	vec3 p0 = vec3(-4.0, -5.0, 7.0);
	vec3 p1 = vec3(4.0, -5.0, 7.0);
	vec3 p2 = vec3(4.0, -5.0, -7.0);
	vec3 p3 = vec3(-4.0, -5.0, -7.0);

	vec3 p4 = vec3(-4.0, -1.0, 7.0);
	vec3 p5 = vec3(4.0, -1.0, 7.0);
	vec3 p6 = vec3(4.0, -1.0, -7.0);
	vec3 p7 = vec3(-4.0, -1.0, -7.0);

	// Define triangles for the cubic room with UVs
	std::vector<Triangle> cubeTriangles = {
		// Bottom face (Floor with checkerboard texture)
		Triangle(p0, p1, p2, 2, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(4, 0), vec2(4, 2), false, true, 0), // Material 2, ColorMapIndex 0
		Triangle(p0, p2, p3, 2, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(4, 2), vec2(0, 2), false, true, 0),

		// Top face (Ceiling - reflective surface, not the light itself)
		Triangle(p6, p5, p4, 3, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1), // Material 3
		Triangle(p7, p6, p4, 3, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),

		// Front face (Yellow wall - same material as small cube)
		Triangle(p5, p1, p0, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1), // Material 1
		Triangle(p4, p5, p0, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),

		// Back face (Blue wall)
		Triangle(p3, p2, p6, 0, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1), // Material 0
		Triangle(p3, p6, p7, 0, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),

		// Left face (Generic white/grey wall)
		Triangle(p0, p3, p7, 4, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1), // Material 4
		Triangle(p0, p7, p4, 4, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),

		// Right face (Generic white/grey wall)
		Triangle(p6, p2, p1, 4, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1), // Material 4
		Triangle(p5, p6, p1, 4, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1)
	};

	// LED light vertices on the ceiling (Main light)
	vec3 lp0 = vec3(-3.5, -1.01, 6.0);
	vec3 lp1 = vec3(3.5, -1.01, 6.0);
	vec3 lp2 = vec3(3.5, -1.01, -6.0);
	vec3 lp3 = vec3(-3.5, -1.01, -6.0);

	// Define triangles for the main LED light
	std::vector<Triangle> lightTriangles = {
		Triangle(lp0, lp1, lp2, 10, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1), // Material 10
		Triangle(lp0, lp2, lp3, 10, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1) // Material 10
	};

	// --- Adding the small metallic cube object ---
	vec3 small_cube_center = vec3(-2.0, -4.0, -3.0);
	float small_cube_size = 0.25f;

	vec3 scc_p0 = small_cube_center + vec3(-small_cube_size, -small_cube_size, small_cube_size);
	vec3 scc_p1 = small_cube_center + vec3(small_cube_size, -small_cube_size, small_cube_size);
	vec3 scc_p2 = small_cube_center + vec3(small_cube_size, -small_cube_size, -small_cube_size);
	vec3 scc_p3 = small_cube_center + vec3(-small_cube_size, -small_cube_size, -small_cube_size);

	vec3 scc_p4 = small_cube_center + vec3(-small_cube_size, small_cube_size, small_cube_size);
	vec3 scc_p5 = small_cube_center + vec3(small_cube_size, small_cube_size, small_cube_size);
	vec3 scc_p6 = small_cube_center + vec3(small_cube_size, small_cube_size, -small_cube_size);
	vec3 scc_p7 = small_cube_center + vec3(-small_cube_size, small_cube_size, -small_cube_size);

	std::vector<Triangle> smallCubeTriangles = {
		// All faces use the yellow reflective material (Material 1)
		Triangle(scc_p0, scc_p1, scc_p2, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(scc_p0, scc_p2, scc_p3, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),
		Triangle(scc_p6, scc_p5, scc_p4, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(scc_p7, scc_p6, scc_p4, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),
		Triangle(scc_p5, scc_p1, scc_p0, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(scc_p4, scc_p5, scc_p0, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),
		Triangle(scc_p3, scc_p2, scc_p6, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(scc_p3, scc_p6, scc_p7, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),
		Triangle(scc_p0, scc_p3, scc_p7, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(scc_p0, scc_p7, scc_p4, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),
		Triangle(scc_p6, scc_p2, scc_p1, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(scc_p5, scc_p6, scc_p1, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1)
	};

	// --- Red Emissive Cube (Smaller, Brighter, Moved to Left) ---
	// Glass sphere center: (1.5, -3.0, -3.0)
	vec3 emissive_cube_center = vec3(0.0, -3.5, -2.5); // Moved X to the left, slightly forward in Z
	float emissive_cube_size = 0.1f; // **Smaller!** (Side length 0.4)

	vec3 ecc_p0 = emissive_cube_center + vec3(-emissive_cube_size, -emissive_cube_size, emissive_cube_size);
	vec3 ecc_p1 = emissive_cube_center + vec3(emissive_cube_size, -emissive_cube_size, emissive_cube_size);
	vec3 ecc_p2 = emissive_cube_center + vec3(emissive_cube_size, -emissive_cube_size, -emissive_cube_size);
	vec3 ecc_p3 = emissive_cube_center + vec3(-emissive_cube_size, -emissive_cube_size, -emissive_cube_size);

	vec3 ecc_p4 = emissive_cube_center + vec3(-emissive_cube_size, emissive_cube_size, emissive_cube_size);
	vec3 ecc_p5 = emissive_cube_center + vec3(emissive_cube_size, emissive_cube_size, emissive_cube_size);
	vec3 ecc_p6 = emissive_cube_center + vec3(emissive_cube_size, emissive_cube_size, -emissive_cube_size);
	vec3 ecc_p7 = emissive_cube_center + vec3(-emissive_cube_size, emissive_cube_size, -emissive_cube_size);

	std::vector<Triangle> redEmissiveCubeTriangles = {
		// All faces use the new Bright Red Emissive material (Material 11)
		Triangle(ecc_p0, ecc_p1, ecc_p2, 11, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(ecc_p0, ecc_p2, ecc_p3, 11, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),
		Triangle(ecc_p6, ecc_p5, ecc_p4, 11, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(ecc_p7, ecc_p6, ecc_p4, 11, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),
		Triangle(ecc_p5, ecc_p1, ecc_p0, 11, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(ecc_p4, ecc_p5, ecc_p0, 11, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),
		Triangle(ecc_p3, ecc_p2, ecc_p6, 11, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(ecc_p3, ecc_p6, ecc_p7, 11, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),
		Triangle(ecc_p0, ecc_p3, ecc_p7, 11, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(ecc_p0, ecc_p7, ecc_p4, 11, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),
		Triangle(ecc_p6, ecc_p2, ecc_p1, 11, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(ecc_p5, ecc_p6, ecc_p1, 11, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1)
	};

	HDRI env = loadHDRI("assets/sky2.exr");


	// Combine all vectors into one
	std::vector<Triangle> sceneTriangles;
	sceneTriangles.reserve(cubeTriangles.size() + lightTriangles.size() + smallCubeTriangles.size() + redEmissiveCubeTriangles.size());
	sceneTriangles.insert(sceneTriangles.end(), cubeTriangles.begin(), cubeTriangles.end());
	sceneTriangles.insert(sceneTriangles.end(), lightTriangles.begin(), lightTriangles.end());
	sceneTriangles.insert(sceneTriangles.end(), smallCubeTriangles.begin(), smallCubeTriangles.end());
	sceneTriangles.insert(sceneTriangles.end(), redEmissiveCubeTriangles.begin(), redEmissiveCubeTriangles.end()); // NEW emissive cube added

	Scene scene;
	scene.triangles = sceneTriangles;
	scene.spheres = scatteredSpheres;
	scene.mats = materials;
	scene.colorMaps = colorMaps;
	scene.hdr = env;

	// Return the Scene
	return scene;
}
inline Scene testSceneModel()
{
	using namespace glm;
	auto texture = LoadImage("assets/Cocacola.jpg");
	std::vector<Image> textures;
	textures.push_back(texture);

	auto triangles = loadFromObj("assets/cup.obj");
	Triangle::AssignColorIndexForMesh(triangles, 0);
	Triangle::AssignMaterialIndexForMesh(triangles, 3);
	Triangle::ActivateTextureForMesh(triangles);
	std::cout << "coco cola Size;" << triangles.size() << "\n";

	std::vector<Material> materials = {
		Material(vec3(0.99, 0.99, 0.99), 0.01, vec3(0.0f), 0.001, 0.0, 0.0), // Floor
		Material(vec3(0.9, 0.69, 0.19), 0.0, vec3(1.0f), 0.3, 0.0, 0.0),    // Metallic
		Material(vec3(0.99, 0.99, 0.88), 0.99, vec3(1.0f), 0.0, 1, 0.0),    // Emissive (white)
		Material(vec3(0.9, 0.8, 0.9), 0.99, vec3(0.0f), 0.01, 0.0, 0),      // Coco material
		Material(vec3(0.9f), 0.1f, vec3(0.0f), 0.5, 0.0, 0.0),              // Wall material (diffuse white)
		Material(vec3(1.0f), 0.0f, vec3(1.0f, 0.8f, 0.4f), 0.0, 25.0, 0.0)   // Emissive cube (warm light)
	};

	// Ground plane
	std::vector<Triangle> groundPlane = {
		Triangle(vec3(-200.0, -10.5, -200.0), vec3(-200.0, -10.5, 200.0), vec3(200.0, -10.5, -200.0),
				 0, vec3(0.0), vec3(0.0), vec3(0.0), vec2(0.0), vec2(0.0), vec2(0.0), false),
		Triangle(vec3(-200.0, -10.5, 200.0), vec3(200.0, -10.5, 200.0), vec3(200.0, -10.5, -200.0),
				 0, vec3(0.0), vec3(0.0), vec3(0.0), vec2(0.0), vec2(0.0), vec2(0.0), false)
	};

	// Enclosing walls (forming a room)
	std::vector<Triangle> walls;
	const float wallHeight = 66.0f;
	const float wallPos = 40.0f;

	// Left wall (x = -wallPos)
	walls.emplace_back(vec3(-wallPos, -10.5, -wallPos), vec3(-wallPos, -10.5, wallPos), vec3(-wallPos, wallHeight, wallPos), 4);
	walls.emplace_back(vec3(-wallPos, -10.5, -wallPos), vec3(-wallPos, wallHeight, wallPos), vec3(-wallPos, wallHeight, -wallPos), 4);

	// Right wall (x = wallPos)
	walls.emplace_back(vec3(wallPos, -10.5, wallPos), vec3(wallPos, -10.5, -wallPos), vec3(wallPos, wallHeight, -wallPos), 4);
	walls.emplace_back(vec3(wallPos, -10.5, wallPos), vec3(wallPos, wallHeight, -wallPos), vec3(wallPos, wallHeight, wallPos), 4);

	// Back wall (z = -wallPos)
	walls.emplace_back(vec3(-wallPos, -10.5, -wallPos), vec3(wallPos, -10.5, -wallPos), vec3(wallPos, wallHeight, -wallPos), 4);
	walls.emplace_back(vec3(-wallPos, -10.5, -wallPos), vec3(wallPos, wallHeight, -wallPos), vec3(-wallPos, wallHeight, -wallPos), 4);

	// Front wall (z = wallPos)
	walls.emplace_back(vec3(wallPos, -10.5, wallPos), vec3(-wallPos, -10.5, wallPos), vec3(-wallPos, wallHeight, wallPos), 4);
	walls.emplace_back(vec3(wallPos, -10.5, wallPos), vec3(-wallPos, wallHeight, wallPos), vec3(wallPos, wallHeight, wallPos), 4);

	// Emissive cube (centered at (0, 3, 0) with size 2)
	std::vector<Triangle> lightCube;
	const float cubeSize = 4.0f;
	const vec3 cubeCenter(20.0f, 10.0f, 10.0f);

	// Front face
	lightCube.emplace_back(cubeCenter + vec3(-cubeSize, -cubeSize, cubeSize),
		cubeCenter + vec3(cubeSize, -cubeSize, cubeSize),
		cubeCenter + vec3(cubeSize, cubeSize, cubeSize), 5);
	lightCube.emplace_back(cubeCenter + vec3(-cubeSize, -cubeSize, cubeSize),
		cubeCenter + vec3(cubeSize, cubeSize, cubeSize),
		cubeCenter + vec3(-cubeSize, cubeSize, cubeSize), 5);

	// Back face
	lightCube.emplace_back(cubeCenter + vec3(-cubeSize, -cubeSize, -cubeSize),
		cubeCenter + vec3(-cubeSize, cubeSize, -cubeSize),
		cubeCenter + vec3(cubeSize, cubeSize, -cubeSize), 5);
	lightCube.emplace_back(cubeCenter + vec3(-cubeSize, -cubeSize, -cubeSize),
		cubeCenter + vec3(cubeSize, cubeSize, -cubeSize),
		cubeCenter + vec3(cubeSize, -cubeSize, -cubeSize), 5);

	// Left face
	lightCube.emplace_back(cubeCenter + vec3(-cubeSize, -cubeSize, -cubeSize),
		cubeCenter + vec3(-cubeSize, -cubeSize, cubeSize),
		cubeCenter + vec3(-cubeSize, cubeSize, cubeSize), 5);
	lightCube.emplace_back(cubeCenter + vec3(-cubeSize, -cubeSize, -cubeSize),
		cubeCenter + vec3(-cubeSize, cubeSize, cubeSize),
		cubeCenter + vec3(-cubeSize, cubeSize, -cubeSize), 5);

	// Right face
	lightCube.emplace_back(cubeCenter + vec3(cubeSize, -cubeSize, cubeSize),
		cubeCenter + vec3(cubeSize, -cubeSize, -cubeSize),
		cubeCenter + vec3(cubeSize, cubeSize, -cubeSize), 5);
	lightCube.emplace_back(cubeCenter + vec3(cubeSize, -cubeSize, cubeSize),
		cubeCenter + vec3(cubeSize, cubeSize, -cubeSize),
		cubeCenter + vec3(cubeSize, cubeSize, cubeSize), 5);

	// Top face
	lightCube.emplace_back(cubeCenter + vec3(-cubeSize, cubeSize, cubeSize),
		cubeCenter + vec3(cubeSize, cubeSize, cubeSize),
		cubeCenter + vec3(cubeSize, cubeSize, -cubeSize), 5);
	lightCube.emplace_back(cubeCenter + vec3(-cubeSize, cubeSize, cubeSize),
		cubeCenter + vec3(cubeSize, cubeSize, -cubeSize),
		cubeCenter + vec3(-cubeSize, cubeSize, -cubeSize), 5);

	// Bottom face
	lightCube.emplace_back(cubeCenter + vec3(-cubeSize, -cubeSize, -cubeSize),
		cubeCenter + vec3(cubeSize, -cubeSize, -cubeSize),
		cubeCenter + vec3(cubeSize, -cubeSize, cubeSize), 5);
	lightCube.emplace_back(cubeCenter + vec3(-cubeSize, -cubeSize, -cubeSize),
		cubeCenter + vec3(cubeSize, -cubeSize, cubeSize),
		cubeCenter + vec3(-cubeSize, -cubeSize, cubeSize), 5);

	HDRI env = loadHDRI("assets/sky2.exr");
	Scene scene;
	scene.triangles = triangles;
	scene.triangles.insert(scene.triangles.end(), groundPlane.begin(), groundPlane.end());
	scene.triangles.insert(scene.triangles.end(), walls.begin(), walls.end());
	scene.triangles.insert(scene.triangles.end(), lightCube.begin(), lightCube.end());
	scene.mats = materials;
	scene.colorMaps = textures;
	scene.hdr = env;

	return scene;
}
inline Scene testSceneModel2()
{
	using namespace glm;
	auto triangles = loadFromObj("assets/coca.obj");
	std::cout << "coco cola Size;" << triangles.size() << "\n";
	std::vector<Material> materials = {
		Material(vec3(0.99, 0.99, 0.99), 0.1, vec3(0.0f), 0.01, 0.0, 0.0), // Red
		Material(vec3(0.9, 0.69, 0.19), 0.0, vec3(1.0f), 0.3, 0.0, 0.0), // Green
		Material(vec3(0.99, 0.99, 0.88), 0.99, vec3(1.0f), 0.0,1.3, 0.0), // Blue
		Material(vec3(0.25, 0.25, 0.25), 0.99, vec3(0.0f), 0.0, 0.0, 0), // coco

	};

	// Define spheres inside the cube and move them up
	std::vector<Sphere> scatteredSpheres = {
		//Sphere(vec3(-1.0, 3.0, 1.0), 0.6, 0), // Red sphere
		//Sphere(vec3(2.0, 3.0, -1.0), 0.5, 1), // Green sphere
		//Sphere(vec3(-2000.0, 5000.0, 0.0), 2000, 2) // Blue sphere
	};
	// Define ground plane with two triangles
	std::vector<Triangle> groundPlane = {
		// Triangle 1
		Triangle(vec3(-200.0, 0.0, -200.0), vec3(-200.0, 0.0, 200.0), vec3(200.0, 0.0, -200.0),0,vec3(0.0),vec3(0.0),vec3(0.0),vec2(0.0),vec2(0.0),vec2(0.0),false), // Ground triangle 1

		// Triangle 2
		Triangle(vec3(-200.0, 0.0, 200.0), vec3(200.0, 0.0, 200.0), vec3(200.0, -0.0, -200.0), 0,vec3(0.0),vec3(0.0),vec3(0.0),vec2(0.0),vec2(0.0),vec2(0.0),false) // Ground triangle 2
	};


	Scene scene;
	scene.triangles = triangles;
	scene.triangles.insert(scene.triangles.end(), groundPlane.begin(), groundPlane.end()); // Add ground triangle to existing triangles
	scene.spheres = scatteredSpheres;
	scene.mats = materials;

	// Return the Scene
	return scene;
}
inline Scene testSceneDragon()
{
	using namespace glm;
	Image checker_board = LoadImage("assets/board.png");
	std::vector<Image> colorMaps;
	colorMaps.push_back(checker_board); // Index 0: checker_board

	// Define materials
	std::vector<Material> materials = {
		Material(vec3(0.99, 0.99, 0.79), 0.7, vec3(0.0f), 0.1, 0.0, 0.0), // 0: Blue (for the wall)
		Material(vec3(0.9, 0.69, 0.69), 0.99, vec3(0.0f), 0.01, 0.0, 0.0), // 1: Yellow (highly reflective, for small cube and wall)
		Material(vec3(0.9, 0.9, 0.5), 0, vec3(0.0f), 0.00001, 0.0, 0.0),// 2: White (floor - will use checkerboard)
		Material(vec3(0.9, 0.9, 0.9), 0.1, vec3(0.0f), 0.5, 0.0, 0.0),// 3: White (ceiling - for the actual room ceiling geometry, not the light)
		Material(vec3(0.9, 0.9, 0.9), 0.6, vec3(0.0f), 0.8, 0.0, 0),// 4: Generic opaque white/grey (for walls)
		Material(vec3(0.9, 0.9, 0.9), 0.9, vec3(0.0f), 0.1, 0.0, 0.0),// 5: Chrome (highly metallic, low roughness sphere)
		Material(vec3(0.99, 0.99, 0.99), 0.0, vec3(0.0f), 0.001, 0.0, 1.1),// 6: Clear Glass (low roughness, high eta)
		Material(vec3(1.0, 1.0, 1.0), 0.0, vec3(1.0f, 1.0f, 1.0f), 0.0, 10.0, 0.0),// 7: Intense Orange Emissive (for point light)
		Material(vec3(0.0, 1.0, 0.0), 0.0, vec3(0.0f, 1.0f, 0.0f), 0.0, 8.0, 0.0), // 8: Intense Green Emissive (for point light)
		Material(vec3(2.0, -2.2, 1.0), 0.0, vec3(2.0f, -2.2f, 1.0f), 0.0, 20.0, 0.0),// 9: Intense Red Emissive (for point light)
		Material(vec3(1.0, 1.0, 1.0), 0.0, vec3(0.6f, 0.6f, 0.6f), 0.0, 1, 0.0),// 10: Emissive white (main ceiling light)
		Material(vec3(1.0, 0.0, 0.0), 0.0, vec3(1.0f, 0.0f, 0.0f), 0.0, 55.0 , 0.0),// 11: Bright Red Emissive (for the new cube)
		Material(vec3(1.0, 1.0, 1.0), 1, vec3(0.0f), 0.5, 0.0, 0) // 12: NEW: Refractive Dragon Material (Glass)
	};

	// Define spheres inside the cube
	std::vector<Sphere> scatteredSpheres = {
		Sphere(vec3(1.5, -15.0, -3.0), 2, 7), // Large Glass sphere (Material 6)
		Sphere(vec3(-2.5, -4.2, 3.0), 0.4, 5), // Chrome sphere (Material 5)
		// Super small and bright lights
		Sphere(vec3(-1.0, -1.5, 0.5), 0.05, 7), // Intense Orange Point Light Sphere (Material 7)
		Sphere(vec3(0.0, -1.8, -4.0), 0.05, 8), // Intense Green Point Light Sphere (Material 8)
		Sphere(vec3(2.0, -2.2, 1.0), 0.05, 9) // Intense Red Point Light Sphere (Material 9)
	};

	// --- Room Scaling Variable ---
	float roomScale = 6.0f; // Start with 5x scaling as requested
	auto zoffset = vec3(0.0f, 8.0f, -25.0f);
	// Cube vertices (for the room) - Scaled
	vec3 p0 = vec3(-4.0, -5.0, 7.0) * roomScale + zoffset;
	vec3 p1 = vec3(4.0, -5.0, 7.0) * roomScale + zoffset;
	vec3 p2 = vec3(4.0, -5.0, -7.0) * roomScale + zoffset;
	vec3 p3 = vec3(-4.0, -5.0, -7.0) * roomScale + zoffset;

	vec3 p4 = vec3(-4.0, -1.0, 7.0) * roomScale + zoffset;
	vec3 p5 = vec3(4.0, -1.0, 7.0) * roomScale + zoffset;
	vec3 p6 = vec3(4.0, -1.0, -7.0) * roomScale + zoffset;
	vec3 p7 = vec3(-4.0, -1.0, -7.0) * roomScale + zoffset;

	// Define triangles for the cubic room with UVs
	std::vector<Triangle> cubeTriangles = {
		// Floor, ceiling, and walls... (same as testSceneBox)
		// Bottom face (Floor with checkerboard texture)
		Triangle(p0, p1, p2, 2, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(4, 0), vec2(4, 2), false, true, 0),
		Triangle(p0, p2, p3, 2, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(4, 2), vec2(0, 2), false, true, 0),

		// Top face (Ceiling)
		Triangle(p6, p5, p4, 3, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(p7, p6, p4, 3, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),

		// Front face (Yellow wall)
		Triangle(p5, p1, p0, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(p4, p5, p0, 1, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),

		// Back face (Blue wall)
		Triangle(p3, p2, p6, 0, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(p3, p6, p7, 0, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),

		// Left face (Generic white/grey wall)
		Triangle(p0, p3, p7, 4, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(p0, p7, p4, 4, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1),

		// Right face (Generic white/grey wall)
		Triangle(p6, p2, p1, 4, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(p5, p6, p1, 4, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1)
	};

	// LED light vertices on the ceiling - Scaled
	vec3 lp0 = vec3(-3.5, -1.01, 6.0) * roomScale + zoffset;
	vec3 lp1 = vec3(3.5, -1.01, 6.0) * roomScale + zoffset;
	vec3 lp2 = vec3(3.5, -1.01, -6.0) * roomScale + zoffset;
	vec3 lp3 = vec3(-3.5, -1.01, -6.0) * roomScale + zoffset;

	// Define triangles for the main LED light
	std::vector<Triangle> lightTriangles = {
		Triangle(lp0, lp1, lp2, 10, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1), false, false, -1),
		Triangle(lp0, lp2, lp3, 10, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1), false, false, -1)
	};

	// --- Load the Dragon mesh ---
	auto dragonTriangles = loadFromObj("assets/Dragon.obj");

	// Assign the refractive material (index 12) to the dragon
	Triangle::AssignMaterialIndexForMesh(dragonTriangles, 12);
	// Deactivate texture mapping for the dragon if it has any UVs from the obj
	Triangle::DeactivateTextureForMesh(dragonTriangles);

	HDRI env = loadHDRI("assets/sky2.exr");

	// Combine all triangles into one vector
	std::vector<Triangle> sceneTriangles;
	sceneTriangles.reserve(cubeTriangles.size() + lightTriangles.size() + dragonTriangles.size());
	sceneTriangles.insert(sceneTriangles.end(), cubeTriangles.begin(), cubeTriangles.end());
	sceneTriangles.insert(sceneTriangles.end(), lightTriangles.begin(), lightTriangles.end());
	sceneTriangles.insert(sceneTriangles.end(), dragonTriangles.begin(), dragonTriangles.end()); // Add the dragon

	Scene scene;
	scene.triangles = sceneTriangles;
	scene.spheres = scatteredSpheres;
	scene.mats = materials;
	scene.colorMaps = colorMaps;
	scene.hdr = env;

	// Return the Scene
	return scene;
}