#pragma once

#include"primitives.h"
#include "modelLoader.h"
#include "imageLoader.h"
#include <iostream>
inline Scene testSceneBox()
{
	using namespace glm;
	Image checker_board = LoadImage("assets/good.jpg");
	std::vector<Image> colorMaps;
	colorMaps.push_back(checker_board);

	// Define materials
	std::vector<Material> materials = {
		Material(vec3(0.39, 0.29, 0.9), 0.0, vec3(0.0f), 0.405, 0.0, 0.0), // Red
		Material(vec3(0.9, 0.69, 0.19), 0.0, vec3(0.0f), 0.005, 0.0, 0.0), // Green
		Material(vec3(0.99, 0.99, 0.79), 0.7, vec3(0.0f), 0.1, 0.0, 0.0), // Blue
		Material(vec3(0.9, 0.69, 0.69), 0.99, vec3(1.0, 0.0, 0.0), 0.01, 0.0, 0.0), // Yellow
		Material(vec3(0.9, 0.9, 0.5), 0, vec3(0.0f), 0.00001, 0.0, 0.0), // White (floor)
		Material(vec3(0.9, 0.9, 0.9), 0.1, vec3(0.0f), 0.5, 0.0, 0.0), // White (ceiling)
		Material(vec3(1.0, 1.0, 1.0), 0.0, vec3(0.6f, 0.6f, 0.6f), 0.0, 2.0, 0.0), // Emissive white
		Material(vec3(0.9, 0.9, 0.9), 0.6, vec3(0.0f), 0.8, 0.0, 0) // Plane
	};

	// Define spheres inside the cube
	std::vector<Sphere> scatteredSpheres = {
		Sphere(vec3(-1.0, -4.0, 1.0), 0.6, 0), // Red sphere
		Sphere(vec3(2.0, -4.0, -1.0), 0.5, 1), // Green sphere
		Sphere(vec3(1.0, -3.0, 1.0), 0.5, 2) // Blue sphere
	};

	// Cube vertices
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
		// Bottom face
		Triangle(p0, p1, p2,4, vec3(0.0f), vec3(0.0f), vec3(0.0f),vec2(0, 0), vec2(4, 0), vec2(4, 2),false,true,0),
		Triangle(p0, p2, p3,4, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(4, 2), vec2(0, 2),false,true,0),

		// Top face
		Triangle(p6, p5, p4,5,vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1)),
		Triangle(p7, p6, p4,5, vec3(0.0f), vec3(0.0f), vec3(0.0f),vec2(0, 0), vec2(1, 1), vec2(0, 1)),

		// Front face
		Triangle(p5, p1, p0,3,vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1)),
		Triangle(p4, p5, p0,3,vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1)),

		// Back face
		Triangle(p3, p2, p6,2, vec3(0.0f), vec3(0.0f), vec3(0.0f),vec2(0, 0), vec2(1, 0), vec2(1, 1)),
		Triangle(p3, p6, p7,2, vec3(0.0f), vec3(0.0f), vec3(0.0f),vec2(0, 0), vec2(1, 1), vec2(0, 1)),

		// Left face
		Triangle(p0, p3, p7,0, vec3(0.0f), vec3(0.0f), vec3(0.0f),vec2(0, 0), vec2(1, 0), vec2(1, 1)),
		Triangle(p0, p7, p4,0, vec3(0.0f), vec3(0.0f), vec3(0.0f),vec2(0, 0), vec2(1, 1), vec2(0, 1)),

		// Right face
		Triangle(p6, p2, p1,1, vec3(0.0f), vec3(0.0f), vec3(0.0f),vec2(0, 0), vec2(1, 0), vec2(1, 1)),
		Triangle(p5, p6, p1,1, vec3(0.0f), vec3(0.0f), vec3(0.0f),vec2(0, 0), vec2(1, 1), vec2(0, 1))
	};

	// LED light vertices on the ceiling
	vec3 lp0 = vec3(-3.5, -1.01, 6.0);
	vec3 lp1 = vec3(3.5, -1.01, 6.0);
	vec3 lp2 = vec3(3.5, -1.01, -6.0);
	vec3 lp3 = vec3(-3.5, -1.01, -6.0);

	// Define triangles for the LED light with UVs
	std::vector<Triangle> lightTriangles = {
		Triangle(lp0, lp1, lp2,6,vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1)),
		Triangle(lp0, lp2, lp3,6, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 1), vec2(0, 1))
	};

	// Define vertices for a plane
	vec3 v0 = vec3(-1.0, -4.5, 0.0); // Bottom-left
	vec3 v1 = vec3(1.0, -4.5, 0.0);  // Bottom-right
	vec3 v2 = vec3(1.0, -4.5, -2.0); // Top-right
	vec3 v3 = vec3(-1.0, -4.5, -2.0); // Top-left

	// Define triangles for the plane with UVs
	std::vector<Triangle> planeTriangles = {
		Triangle(v0, v1, v2,7, vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0, 0), vec2(1, 0), vec2(1, 1)),
		Triangle(v0, v2, v3,7, vec3(0.0f), vec3(0.0f), vec3(0.0f),vec2(0, 0), vec2(1, 1), vec2(0, 1))
	};

	HDRI env = loadHDRI("assets/sky2.exr");


	// Combine all vectors into one
	std::vector<Triangle> sceneTriangles;
	sceneTriangles.reserve(cubeTriangles.size() + lightTriangles.size() + planeTriangles.size()); // Reserve space for efficiency
	sceneTriangles.insert(sceneTriangles.end(), cubeTriangles.begin(), cubeTriangles.end());
	sceneTriangles.insert(sceneTriangles.end(), lightTriangles.begin(), lightTriangles.end());
	sceneTriangles.insert(sceneTriangles.end(), planeTriangles.begin(), planeTriangles.end());

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
