#pragma once

#include "primitives.h"
#include "modelLoader.h"
#include "imageLoader.h"
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
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

	// Faces are listed clockwise seen from outside; emit them counter-clockwise (outward,
	// the glTF convention) so front/back-face tests work for transmissive cubes.
	auto pushQuad = [&](int a, int b, int c, int d) {
		cubeTriangles.push_back(makeTri(p[a], p[c], p[b], uv0, uv2, uv1));
		cubeTriangles.push_back(makeTri(p[a], p[d], p[c], uv0, uv3, uv2));
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
	scene.addInstance(scene.addMesh(std::move(sceneTriangles)));
	// scene.spheres = scatteredSpheres;
	scene.mats = materials;
	scene.colorMaps = colorMaps;
	scene.hdr = env;

	// Return the Scene
	return scene;
}

inline Scene testSceneGLTF() {
	using namespace glm;

	Scene scene;
	// Sponza's root node scales by 0.008; undo it so the scene keeps its original units.
	loadGLTF("assets/glTF/Sponza.gltf", scene, scale(mat4(1.0f), vec3(125.0f)));
	HDRI env = imgutl::loadHDRI("assets/sky2.exr");
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
	// One cube mesh, five instances with their own emissive material.
	int cubeMesh = -1;
	for (int i = 0; i < 5; ++i) {
		Material cubeMat;
		cubeMat.albedo = colors[i];
		cubeMat.roughness = 0.5f;
		cubeMat.metallic = 0.0f;
		cubeMat.emission_power = 15;
		cubeMat.emission_color = colors[i];

		int matIndex = static_cast<int>(scene.mats.size());
		scene.mats.push_back(cubeMat);

		if (cubeMesh == -1)
			cubeMesh = scene.addMesh(makeCube(vec3(0.0f), cubeSize, matIndex));
		scene.addInstance(cubeMesh, translate(mat4(1.0f), startPos + spacing * float(i)), matIndex);
	}

	return scene;
}

// Khronos SimpleInstancing: one cube mesh, 125 instances via EXT_mesh_gpu_instancing.
inline Scene testSceneInstancing() {
	using namespace glm;

	Scene scene;
	// Grid spans roughly [0, 12]; centre it in front of the default camera.
	loadGLTF("assets/SimpleInstancing/SimpleInstancing.gltf", scene, translate(mat4(1.0f), vec3(-6.0f, -6.0f, 20.0f)));
	scene.hdr = imgutl::loadHDRI("assets/sky2.exr");
	return scene;
}

// Loads a glTF and, if it contains a camera, views the scene from it.
inline Scene testSceneFromGLTFCamera(const std::string& path) {
	using namespace glm;

	Scene scene;
	loadGLTF(path, scene);
	if (scene.camera)
	{
		// Kiran's camera sits at the origin looking down +z (left-handed);
		// glTF cameras look down -z, so flip z after moving into the camera's space.
		scene.transformInstances(scale(mat4(1.0f), vec3(1.0f, 1.0f, -1.0f)) * inverse(*scene.camera));
	}
	scene.hdr = imgutl::loadHDRI("assets/sky2.exr");
	return scene;
}

// EXT_mesh_gpu_instancing spec sample: ~1900 teapots from one mesh across a nested node hierarchy.
inline Scene testSceneTeapots() { return testSceneFromGLTFCamera("assets/teapots_galore/teapots_galore.gltf"); }

// Khronos NodePerformanceTest: 10,000 nodes, each with a unique mesh and material.
inline Scene testSceneNodes() { return testSceneFromGLTFCamera("assets/NodePerformanceTest/NodePerformanceTest.gltf"); }

// Amazon Lumberyard Bistro interior (glTF, 512px base-color textures). Lit by its
// emissive lamps plus the sky through the windows. No camera in the file, so put
// Kiran's camera at eye height mid-room; z is flipped for Kiran's left-handed view.
inline Scene testSceneBistro() {
	using namespace glm;

	Scene scene;
	// rotate(+90° about Y) turns the starting view 90° to the left.
	loadGLTF("assets/BistroInterior/BistroInterior.gltf", scene,
		rotate(mat4(1.0f), radians(90.0f), vec3(0.0f, 1.0f, 0.0f)) *
		scale(mat4(1.0f), vec3(1.0f, 1.0f, -1.0f)) * translate(mat4(1.0f), vec3(-6.0f, -1.7f, 4.0f)));
	scene.hdr = imgutl::loadHDRI("assets/sky2.exr");
	return scene;
}

// White-furnace check: diffuse cube (albedo 0.5) under a uniform sky of radiance 1.
// The cube is convex, so every bounce escapes to the sky and its pixels must read
// exactly 0.5 for any bounce count >= 2. `size` tests scale robustness.
inline Scene testSceneFurnace(float size, float albedo = 0.5f, float metallic = 0.0f, float roughness = 1.0f, float ior = 1.0f) {
	using namespace glm;

	Scene scene;
	Material m{};
	m.albedo = vec3(albedo);
	m.metallic = metallic;
	m.roughness = roughness;
	m.eta = ior; // 1.0: no specular layer, i.e. pure Lambertian (analytic)
	scene.mats.push_back(m);
	int cube = scene.addMesh(makeCube(vec3(0.0f), size, 0));
	scene.addInstance(cube, translate(mat4(1.0f), vec3(0.0f, 0.0f, -1.0f + 2.0f * size)));
	scene.hdr = HDRI{ 4, 2, std::vector<vec4>(8, vec4(1.0f)) };
	return scene;
}

// Light-sampling checks: a diffuse wall (albedo 0.5) at z = 3 facing the camera, black sky.
//   point / spot / dir: one delta light giving unit irradiance at the wall centre, so the
//   centre pixel must read albedo / pi * emissionScale.
//   area: a quad and a sphere light off to the side; with and without NEE must agree.
inline Scene testSceneLights(const std::string& mode) {
	using namespace glm;

	Scene scene;
	Material wall{};
	wall.albedo = vec3(0.5f);
	wall.roughness = 1.0f;
	wall.eta = 1.0f; // pure Lambertian, so the expected value is analytic
	scene.mats.push_back(wall);
	Triangle a{}, b{};
	a.v0 = vec3(-20, -20, 3); a.v1 = vec3(20, -20, 3); a.v2 = vec3(20, 20, 3);
	b.v0 = vec3(-20, -20, 3); b.v1 = vec3(20, 20, 3); b.v2 = vec3(-20, 20, 3);
	scene.addInstance(scene.addMesh({ a, b }));
	scene.hdr = HDRI{ 4, 2, std::vector<vec4>(8, vec4(0.0f)) };

	Light l;
	if (mode == "point") {
		l.type = LightType::Point;
		l.position = vec3(0, 0, 2);
		scene.addLight(l);
	} else if (mode == "spot") {
		l.type = LightType::Spot;
		l.position = vec3(0, 0, 2);
		l.direction = vec3(0, 0, 1);
		l.innerCone = radians(20.0f);
		l.outerCone = radians(30.0f);
		scene.addLight(l);
	} else if (mode == "dir") {
		l.type = LightType::Directional;
		l.direction = vec3(0, 0, 1);
		scene.addLight(l);
	} else if (mode == "sun") {
		// Sized sun (10 deg disc): same irradiance at normal incidence as "dir", so the probes
		// must agree, with light sampling on or off.
		l.type = LightType::Directional;
		l.direction = vec3(0, 0, 1);
		l.angle = radians(10.0f);
		scene.addLight(l);
	} else if (mode != "none") {
		scene.addQuadLight(vec3(1.0f, -0.5f, 2.0f), vec3(1, 0, 0), vec3(0, 1, 0), vec3(1.0f, 0.9f, 0.8f));
		scene.addSphereLight(vec3(-1.5f, 0.5f, 2.2f), 0.3f, vec3(0.6f, 0.8f, 1.0f));
	}
	return scene;
}

// Places the scene's bounding box centred in front of Kiran's default camera (origin,
// looking down +z), with z flipped for Kiran's left-handed view. For files without a camera.
// focusLargest frames only the instances of the mesh with the most triangles (e.g. the
// dragon rather than its ground plane).
inline void frameScene(Scene& scene, float margin = 1.3f, bool focusLargest = false) {
	using namespace glm;
	int focus = -1;
	if (focusLargest)
		for (int m = 0; m < static_cast<int>(scene.meshes.size()); ++m)
			if (focus == -1 || scene.meshes[m].size() > scene.meshes[focus].size())
				focus = m;
	BoundingBox box;
	for (const auto& inst : scene.instances)
		if (focus == -1 || inst.mesh == focus)
		for (const auto& t : scene.meshes[inst.mesh])
			for (const vec3& v : { t.v0, t.v1, t.v2 })
				box.expand(vec3(inst.transform * vec4(v, 1.0f)));
	vec3 center = 0.5f * (box.min + box.max);
	float radius = 0.5f * length(box.max - box.min);
	float distance = margin * radius / std::tan(radians(25.0f)); // default 50 degree fov
	scene.transformInstances(translate(mat4(1.0f), vec3(0.0f, 0.0f, distance - 1.0f)) *
		scale(mat4(1.0f), vec3(1.0f, 1.0f, -1.0f)) * translate(mat4(1.0f), -center));
}

// Khronos DragonAttenuation / AttenuationTest (transmission + volume absorption), sky-lit.
inline Scene testSceneGLTFFramed(const std::string& path, bool focusLargest = false) {
	Scene scene;
	loadGLTF(path, scene);
	frameScene(scene, focusLargest ? 1.0f : 1.3f, focusLargest);
	scene.hdr = imgutl::loadHDRI("assets/sky2.exr");
	return scene;
}

// Analytic refraction + absorption check: a 1 m glass slab (ior 1.5, sigma_a per metre)
// at normal incidence in front of a unit-radiance emitter, black sky. With T = 1 - R,
// R = 0.04, the centre pixel must read emissionScale * T^2 e^(-s) / (1 - R^2 e^(-2s)).
inline Scene testSceneGlassSlab(float sigma, float dispersion = 0.0f) {
	using namespace glm;
	Scene scene;
	Material glass{};
	glass.albedo = vec3(1.0f);
	glass.transmission = 1.0f;
	glass.eta = 1.5f;
	glass.dispersion = dispersion;
	glass.sigma_a = vec3(sigma);
	scene.mats.push_back(glass);
	int slab = scene.addMesh(makeCube(vec3(0.0f), 1.0f, 0));
	scene.addInstance(slab, translate(mat4(1.0f), vec3(0.0f, 0.0f, 3.0f)) * scale(mat4(1.0f), vec3(20.0f, 20.0f, 1.0f)));
	scene.addQuadLight(vec3(-10.0f, -10.0f, 6.0f), vec3(20.0f, 0.0f, 0.0f), vec3(0.0f, 20.0f, 0.0f), vec3(1.0f));
	scene.hdr = HDRI{ 4, 2, std::vector<vec4>(8, vec4(0.0f)) };
	return scene;
}

// Fog checks.
//   absorb: 1 m of purely absorbing fog (sigma_a = 1) in front of a unit emitter, black sky:
//           centre pixel = emissionScale * e^-1.
//   furnace: purely scattering fog block (sigma_s = 1, g = 0.5) under a uniform white sky;
//           energy conservation means it must read ~1 given enough bounces.
//   area: the light-area test filled with scattering fog; NEE on / off must agree.
inline Scene testSceneFog(const std::string& mode) {
	using namespace glm;
	Scene scene;
	if (mode == "area") {
		scene = testSceneLights("area");
		scene.fog = { true, vec3(-5.0f, -5.0f, 0.5f), vec3(5.0f, 5.0f, 2.9f), vec3(0.3f), vec3(0.05f), 0.3f };
		return scene;
	}
	if (mode == "big") {
		// Large, dim emitter: low variance for both estimators.
		scene.addQuadLight(vec3(-1.5f, 1.2f, 0.8f), vec3(3.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 2.0f), vec3(0.2f));
		Material wall{};
		wall.albedo = vec3(0.5f);
		wall.roughness = 1.0f;
		wall.eta = 1.0f;
		scene.mats.push_back(wall);
		Triangle a{}, b{};
		a.matIndex = b.matIndex = static_cast<int>(scene.mats.size()) - 1;
		a.v0 = vec3(-20, -20, 3); a.v1 = vec3(20, -20, 3); a.v2 = vec3(20, 20, 3);
		b.v0 = vec3(-20, -20, 3); b.v1 = vec3(20, 20, 3); b.v2 = vec3(-20, 20, 3);
		scene.addInstance(scene.addMesh({ a, b }));
		scene.fog = { true, vec3(-5.0f, -5.0f, 0.5f), vec3(5.0f, 5.0f, 2.9f), vec3(0.3f), vec3(0.05f), 0.3f };
		scene.hdr = HDRI{ 4, 2, std::vector<vec4>(8, vec4(0.0f)) };
		return scene;
	}
	if (mode == "furnace") {
		scene.fog = { true, vec3(-1.0f, -1.0f, 1.0f), vec3(1.0f, 1.0f, 3.0f), vec3(1.0f), vec3(0.0f), 0.5f };
		scene.hdr = HDRI{ 4, 2, std::vector<vec4>(8, vec4(1.0f)) };
		return scene;
	}
	scene.addQuadLight(vec3(-10.0f, -10.0f, 6.0f), vec3(20.0f, 0.0f, 0.0f), vec3(0.0f, 20.0f, 0.0f), vec3(1.0f));
	scene.fog = { true, vec3(-10.0f, -10.0f, 2.5f), vec3(10.0f, 10.0f, 3.5f), vec3(0.0f), vec3(1.0f), 0.0f };
	scene.hdr = HDRI{ 4, 2, std::vector<vec4>(8, vec4(0.0f)) };
	return scene;
}

// Convex polyhedron cut from the cube [-extent, extent]^3 by the half-spaces
// dot(n, x) <= d, given as (n, d) with n pointing outward. Faces are fan-triangulated
// with flat outward normals.
inline std::vector<Triangle> clipConvex(const std::vector<glm::dvec4>& planes, double extent, int matIndex) {
	using namespace glm;
	using Poly = std::vector<dvec3>;
	const double eps = 1e-10;
	dvec3 c[8];
	for (int k = 0; k < 8; ++k)
		c[k] = dvec3(k & 1 ? extent : -extent, k & 2 ? extent : -extent, k & 4 ? extent : -extent);
	const int q[6][4] = { { 0, 1, 3, 2 }, { 4, 5, 7, 6 }, { 0, 1, 5, 4 }, { 2, 3, 7, 6 }, { 0, 2, 6, 4 }, { 1, 3, 7, 5 } };
	std::vector<Poly> faces;
	for (const auto& f : q)
		faces.push_back({ c[f[0]], c[f[1]], c[f[2]], c[f[3]] });

	for (const dvec4& pl : planes) {
		const dvec3 n(pl);
		std::vector<Poly> kept;
		Poly cut;
		for (const Poly& f : faces) {
			Poly r;
			for (size_t i = 0; i < f.size(); ++i) {
				const dvec3 &a = f[i], &b = f[(i + 1) % f.size()];
				double da = dot(n, a) - pl.w, db = dot(n, b) - pl.w;
				if (da <= eps)
					r.push_back(a);
				if (std::abs(da) <= eps)
					cut.push_back(a);
				if ((da < -eps && db > eps) || (da > eps && db < -eps)) {
					dvec3 p = a + (b - a) * (da / (da - db));
					r.push_back(p);
					cut.push_back(p);
				}
			}
			if (r.size() >= 3)
				kept.push_back(r);
		}
		// Cap polygon: the cut points sorted by angle around their centroid.
		Poly cap;
		for (const dvec3& p : cut) {
			bool dup = false;
			for (const dvec3& o : cap)
				dup = dup || length(p - o) < 1e-9;
			if (!dup)
				cap.push_back(p);
		}
		if (cap.size() >= 3) {
			dvec3 m(0.0);
			for (const dvec3& p : cap)
				m += p / double(cap.size());
			dvec3 u = normalize(cross(n, std::abs(n.x) < 0.9 ? dvec3(1, 0, 0) : dvec3(0, 1, 0))), v = cross(n, u);
			std::sort(cap.begin(), cap.end(), [&](const dvec3& p0, const dvec3& p1) {
				return std::atan2(dot(p0 - m, v), dot(p0 - m, u)) < std::atan2(dot(p1 - m, v), dot(p1 - m, u));
			});
			kept.push_back(cap);
		}
		faces = std::move(kept);
	}

	dvec3 center(0.0);
	size_t count = 0;
	for (const Poly& f : faces)
		for (const dvec3& p : f) {
			center += p;
			++count;
		}
	center /= double(count);
	std::vector<Triangle> tris;
	for (Poly f : faces) {
		Poly g; // drop repeated vertices
		for (const dvec3& p : f)
			if (g.empty() || length(p - g.back()) > 1e-9)
				g.push_back(p);
		while (g.size() > 1 && length(g.front() - g.back()) <= 1e-9)
			g.pop_back();
		if (g.size() < 3)
			continue;
		dvec3 nrm(0.0), m(0.0);
		for (size_t i = 0; i < g.size(); ++i) {
			nrm += cross(g[i], g[(i + 1) % g.size()]);
			m += g[i] / double(g.size());
		}
		if (length(nrm) < 1e-12)
			continue;
		nrm = normalize(nrm);
		if (dot(nrm, m - center) < 0.0) {
			std::reverse(g.begin(), g.end());
			nrm = -nrm;
		}
		for (size_t i = 1; i + 1 < g.size(); ++i) {
			Triangle t{};
			t.v0 = vec3(g[0]); t.v1 = vec3(g[i]); t.v2 = vec3(g[i + 1]);
			t.n0 = t.n1 = t.n2 = vec3(nrm);
			t.hasNormal = true;
			t.matIndex = matIndex;
			tris.push_back(t);
		}
	}
	return tris;
}

// Round brilliant (57 facets) with ideal proportions (Tolkowsky / GIA excellent): table 56%,
// crown angle 34.5 deg, pavilion angle 40.75 deg, star length 50%, lower girdle length 77.5%,
// thin-medium girdle (64-sided). Built as the intersection of the facet half-spaces, so every
// facet has its exact shape. Girdle radius `radius`, girdle bottom at y = 0, table up (+y).
inline std::vector<Triangle> makeDiamond(float radius, int matIndex) {
	using namespace glm;
	const double r = radius, pi = 3.14159265358979323846;
	const double crown = 34.5 * pi / 180.0, pavilion = 40.75 * pi / 180.0;
	const double tableCorner = 0.56 * r, starLength = 0.5, lowerGirdle = 0.775, girdle = 0.03 * r;
	const double tableH = girdle + (r - tableCorner) * std::tan(crown);
	const double eighth = pi / 4.0, sixteenth = pi / 8.0;
	const dvec3 interior(0.0, 0.5 * girdle, 0.0);
	std::vector<dvec4> planes;
	auto add = [&](dvec3 n, const dvec3& p) {
		n = normalize(n);
		if (dot(n, p - interior) < 0.0)
			n = -n;
		planes.push_back(dvec4(n, dot(n, p)));
	};
	auto at = [](double phi, double radial, double y) { return dvec3(radial * std::cos(phi), y, radial * std::sin(phi)); };
	auto tilted = [](double phi, double tilt, double up) { // outward normal tilted `tilt` from vertical
		return dvec3(std::sin(tilt) * std::cos(phi), up * std::cos(tilt), std::sin(tilt) * std::sin(phi));
	};

	for (int k = 0; k < 64; ++k) // girdle
		add(dvec3(std::cos(2.0 * pi * k / 64.0), 0.0, std::sin(2.0 * pi * k / 64.0)), at(2.0 * pi * k / 64.0, r, 0.0));
	add(dvec3(0.0, 1.0, 0.0), dvec3(0.0, tableH, 0.0)); // table
	const double tableEdge = tableCorner * std::cos(sixteenth);
	const double starR = tableEdge + starLength * (r - tableEdge);
	const double starY = girdle + (r - starR * std::cos(sixteenth)) * std::tan(crown); // on the bezel
	const double lowerR = (1.0 - lowerGirdle) * r;
	const double lowerY = -(r - lowerR * std::cos(sixteenth)) * std::tan(pavilion); // on the pavilion main
	for (int k = 0; k < 8; ++k) {
		const double pb = k * eighth, ps = pb + sixteenth, pn = pb + eighth;
		add(tilted(pb, crown, 1.0), at(pb, r, girdle));		  // bezel
		add(tilted(pb, pavilion, -1.0), at(pb, r, 0.0));		  // pavilion main
		const dvec3 edge = at(ps, tableEdge, tableH), star = at(ps, starR, starY);
		add(tilted(ps, std::atan2(tableH - starY, starR - tableEdge), 1.0), edge); // star
		const dvec3 gs = at(ps, r, girdle), q = at(ps, lowerR, lowerY), bs = at(ps, r, 0.0);
		for (double pg : { pb, pn }) {
			const dvec3 gb = at(pg, r, girdle), bb = at(pg, r, 0.0);
			add(cross(gs - gb, star - gb), gb); // upper girdle
			add(cross(bs - bb, q - bb), bb);	// lower girdle
		}
	}
	return clipConvex(planes, 2.0 * r, matIndex);
}

// Views the scene from `eye` towards `target` (y up). Kiran's camera sits at (0, 0, -1) looking
// down +z with +x to the right on screen (left-handed), so move into the right-handed view
// space, flip z and shift onto the camera.
inline void viewFrom(Scene& scene, const glm::vec3& eye, const glm::vec3& target) {
	using namespace glm;
	mat4 m = translate(mat4(1.0f), vec3(0.0f, 0.0f, -1.0f)) * scale(mat4(1.0f), vec3(1.0f, 1.0f, -1.0f)) *
		lookAt(eye, target, vec3(0.0f, 1.0f, 0.0f));
	scene.transformInstances(m);
	// The fog box stays in world space; rays are mapped back into it (any camera angle works).
	scene.fog.toLocal = scene.fog.toLocal * inverse(m);
}

// 4x2 environment: `top` over the upper hemisphere fading to `bottom` below (bilinear).
inline HDRI gradientSky(float top, float bottom) {
	std::vector<glm::vec4> d(8);
	for (int k = 0; k < 4; ++k) {
		d[k] = glm::vec4(top);
		d[4 + k] = glm::vec4(bottom);
	}
	return HDRI{ 4, 2, d };
}

// Coloured version: `top` over the upper hemisphere fading to `bottom` below.
inline HDRI gradientSky(const glm::vec3& top, const glm::vec3& bottom) {
	std::vector<glm::vec4> d(8);
	for (int k = 0; k < 4; ++k) {
		d[k] = glm::vec4(top, 1.0f);
		d[4 + k] = glm::vec4(bottom, 1.0f);
	}
	return HDRI{ 4, 2, d };
}

// Large matte floor (y = 0) centred on `c`, with an upward normal.
inline std::vector<Triangle> makeFloor(const glm::vec3& c, float half, int matIndex) {
	using namespace glm;
	vec3 p[4] = { c + vec3(-half, 0, -half), c + vec3(-half, 0, half), c + vec3(half, 0, half), c + vec3(half, 0, -half) };
	std::vector<Triangle> tris(2);
	tris[0].v0 = p[0]; tris[0].v1 = p[1]; tris[0].v2 = p[2];
	tris[1].v0 = p[0]; tris[1].v1 = p[2]; tris[1].v2 = p[3];
	for (auto& t : tris) {
		t.n0 = t.n1 = t.n2 = vec3(0.0f, 1.0f, 0.0f);
		t.hasNormal = true;
		t.matIndex = matIndex;
	}
	return tris;
}

// Equilateral triangular prism standing on y = 0 (axis +y), apex pointing towards -z.
inline std::vector<Triangle> makePrism(float side, float height, int matIndex) {
	using namespace glm;
	const float R = side / std::sqrt(3.0f); // circumradius
	vec3 base[3] = { { 0.0f, 0.0f, -R }, { -0.5f * side, 0.0f, 0.5f * R }, { 0.5f * side, 0.0f, 0.5f * R } };
	const vec3 interior(0.0f, 0.5f * height, 0.0f);
	std::vector<Triangle> tris;
	auto face = [&](vec3 a, vec3 b, vec3 c) {
		vec3 n = normalize(cross(b - a, c - a));
		if (dot(n, (a + b + c) / 3.0f - interior) < 0.0f) {
			std::swap(b, c);
			n = -n;
		}
		Triangle t{};
		t.v0 = a; t.v1 = b; t.v2 = c;
		t.n0 = t.n1 = t.n2 = n;
		t.hasNormal = true;
		t.matIndex = matIndex;
		tris.push_back(t);
	};
	const vec3 up(0.0f, height, 0.0f);
	face(base[0], base[1], base[2]);
	face(base[0] + up, base[1] + up, base[2] + up);
	for (int k = 0; k < 3; ++k) {
		vec3 a = base[k], b = base[(k + 1) % 3];
		face(a, b, b + up);
		face(a, b + up, a + up);
	}
	return tris;
}

// Newton's prism: an emissive strip shines through a 1 cm slit onto a dense-flint prism at
// minimum deviation; the dispersed beam lands on a white card as a spectrum.
// A black baffle keeps the strip's direct light off the card, so all of the card's light has
// passed through the prism (light tracing carries it). Dark table, black sky.
// fog: the dispersed fan is shown in haze that starts just past the slit (below the slit
// screen's top, so only the beam lights it), seen from a level side camera.
inline Scene testScenePrism(bool fog = false) {
	using namespace glm;
	Scene scene;
	auto matte = [&](vec3 albedo) {
		Material m{};
		m.albedo = albedo;
		m.roughness = 1.0f;
		scene.mats.push_back(m);
		return static_cast<int>(scene.mats.size()) - 1;
	};
	const int table = matte(vec3(0.08f)), white = matte(vec3(0.8f)), black = matte(vec3(0.0f));
	scene.addInstance(scene.addMesh(makeFloor(vec3(0.0f), 3.0f, table)));

	// Dense flint (SF11-like): n_d = 1.785, Abbe number 25.8.
	Material flint{};
	flint.albedo = vec3(1.0f);
	flint.transmission = 1.0f;
	flint.eta = 1.785f;
	flint.dispersion = 20.0f / 25.8f;
	scene.mats.push_back(flint);
	const float side = 0.2f, height = 0.25f, R = side / std::sqrt(3.0f);
	scene.addInstance(scene.addMesh(makePrism(side, height, static_cast<int>(scene.mats.size()) - 1)));

	// Minimum deviation: inside, the ray runs parallel to the base, so it meets the left face
	// at sin i = n sin 30 deg and leaves the right face symmetrically.
	const float i = std::asin(1.785f * 0.5f), a = radians(30.0f) - i;
	const vec3 dIn(std::cos(a), 0.0f, std::sin(a)), dOut(std::cos(a), 0.0f, -std::sin(a));
	const vec3 entry(-0.25f * side, 0.5f * height, -0.25f * R), exitP(0.25f * side, 0.5f * height, -0.25f * R);

	auto quad = [&](vec3 c, vec3 u, vec3 v, int mat) {
		Triangle t0{}, t1{};
		vec3 p0 = c - 0.5f * u - 0.5f * v, p1 = c + 0.5f * u - 0.5f * v, p2 = c + 0.5f * u + 0.5f * v, p3 = c - 0.5f * u + 0.5f * v;
		t0.v0 = p0; t0.v1 = p1; t0.v2 = p2;
		t1.v0 = p0; t1.v1 = p2; t1.v2 = p3;
		t0.matIndex = t1.matIndex = mat;
		return std::vector<Triangle>{ t0, t1 };
	};
	const vec3 up(0.0f, 1.0f, 0.0f);
	// Emissive strip 0.4 m before the entry face, facing the prism (1 cm x 8 cm).
	const vec3 lightC = entry - 0.4f * dIn, across = normalize(cross(up, dIn));
	scene.addQuadLight(lightC - 0.005f * across - vec3(0.0f, 0.04f, 0.0f), 0.01f * across, vec3(0.0f, 0.08f, 0.0f), vec3(1750.0f));
	// White card 0.8 m along the deviated beam, square to it.
	const vec3 cardC = exitP + 0.8f * dOut;
	scene.addInstance(scene.addMesh(quad(cardC, 0.5f * normalize(cross(up, dOut)), vec3(0.0f, 0.3f, 0.0f), white)));
	// Black screen with a 1 cm vertical slit just before the entry face: only a narrow beam
	// reaches the prism, so each wavelength lands as a ~1 cm band and the bands separate
	// (lighting the whole face makes 20 cm wide bands that overlap back into white).
	const vec3 slitC = entry - 0.03f * dIn + vec3(0.0f, 0.175f - 0.5f * height, 0.0f);
	const vec3 inAcross = normalize(cross(up, dIn));
	scene.addInstance(scene.addMesh(quad(slitC + (0.005f + 0.15f) * inAcross, 0.3f * inAcross, vec3(0.0f, 0.35f, 0.0f), black)));
	scene.addInstance(scene.addMesh(quad(slitC - (0.005f + 0.15f) * inAcross, 0.3f * inAcross, vec3(0.0f, 0.35f, 0.0f), black)));
	// Black baffle across the strip's direct line to the card (clear of both beams).
	const vec3 mid = 0.5f * (lightC + cardC);
	scene.addInstance(scene.addMesh(quad(vec3(mid.x, 0.2f, mid.z), 0.25f * normalize(cross(up, cardC - lightC)), vec3(0.0f, 0.4f, 0.0f), black)));

	scene.hdr = gradientSky(0.0f, 0.0f);
	if (fog)
	{
		const float x0 = slitC.x + 0.02f; // just past the slit screen
		scene.fog = { true, vec3(x0, 0.0f, -0.35f), vec3(cardC.x + 0.1f, 0.32f, cardC.z + 0.25f), vec3(1.0f), vec3(0.005f), 0.3f };
		viewFrom(scene, vec3(0.3f, 0.3f, -1.1f), vec3(0.3f, 0.3f, 0.2f)); // level: keeps the fog box axis-aligned
	}
	else
		viewFrom(scene, vec3(0.1f, 0.65f, -0.9f), vec3(0.3f, 0.1f, 0.15f));
	return scene;
}

// White floor (no walls or ceiling) in a thin haze, with an upright round brilliant floating
// just under a small sphere light.
inline Scene testSceneDiamond() {
	using namespace glm;
	Scene scene;
	auto matte = [&](vec3 albedo) {
		Material m{};
		m.albedo = albedo;
		m.roughness = 1.0f;
		scene.mats.push_back(m);
		return static_cast<int>(scene.mats.size()) - 1;
	};
	const int white = matte(vec3(0.75f));
	const float h = 0.4f, H = 0.8f; // half width, height
	std::vector<Triangle> room;
	auto quad = [&](vec3 a, vec3 b, vec3 c, vec3 d, int mat) {
		Triangle t0{}, t1{};
		t0.v0 = a; t0.v1 = b; t0.v2 = c;
		t1.v0 = a; t1.v1 = c; t1.v2 = d;
		t0.matIndex = t1.matIndex = mat;
		room.push_back(t0);
		room.push_back(t1);
	};
	quad({ -h, 0, -h }, { h, 0, -h }, { h, 0, h }, { -h, 0, h }, white); // floor
	scene.addInstance(scene.addMesh(room));

	Material diamond{};
	diamond.albedo = vec3(1.0f);
	diamond.transmission = 1.0f;
	diamond.eta = 2.417f;
	diamond.dispersion = 20.0f / 55.3f; // Abbe number of diamond
	scene.mats.push_back(diamond);
	const float r = 0.12f, pavilion = radians(40.75f);
	// Upright (table up), floating 0.2 m above the floor so the table sits ~0.1 m below the
	// light (the pavilion reaches r tan(pavilion) below the girdle).
	scene.addInstance(scene.addMesh(makeDiamond(r, static_cast<int>(scene.mats.size()) - 1)),
		translate(mat4(1.0f), vec3(0.0f, r * std::tan(pavilion) + 0.2f, 0.0f)));

	// Small, bright sphere light hanging 0.6 m up, above the stone: a small source keeps the
	// dispersed colours apart (fire in the stone, rainbow caustics and beams) instead of
	// blurring them back into white. Radius 2 cm (large enough that glints through the facets
	// converge), intensity 0.0576 (L * pi * r^2): the same power at any radius.
	const float lr = 0.02f;
	scene.addSphereLight(vec3(0.0f, 0.6f, 0.0f), lr, vec3(0.0576f / (pi<float>() * lr * lr)));

	// Thin haze over the floor area: shafts from the light and the stone's dispersed beams.
	scene.fog = { true, vec3(-h, 0.0f, -h), vec3(h, H, h), vec3(0.3f), vec3(0.005f), 0.3f };
	scene.hdr = gradientSky(0.0f, 0.0f);
	viewFrom(scene, vec3(0.0f, 0.4f, -1.1f), vec3(0.0f, 0.4f, 0.0f)); // level: keeps the fog box axis-aligned
	return scene;
}

// A glTF exported from Blender (cameras, punctual lights, area lights baked into emissive
// quads), viewed from the file's camera under the HDRI `skyPath` (empty: a dim constant sky).
inline Scene testSceneBlenderGLTF(const std::string& path, const std::string& skyPath, float sky = 0.02f) {
	using namespace glm;
	Scene scene;
	loadGLTF(path, scene);
	if (scene.camera)
		// glTF cameras look down -z; Kiran's sits at (0, 0, -1) looking down +z with +x to the
		// right on screen (left-handed): flip z and shift onto it.
		scene.transformInstances(translate(mat4(1.0f), vec3(0.0f, 0.0f, -1.0f)) *
			scale(mat4(1.0f), vec3(1.0f, 1.0f, -1.0f)) * inverse(*scene.camera));
	scene.hdr = skyPath.empty() ? gradientSky(sky, 0.0f) : imgutl::loadHDRI(skyPath);
	return scene;
}

// Blender 2.8 splash "Wanderer" (Daniel Bystedt, CC-BY): a traveller in a rocky night
// landscape with a warm key and a red point light (the moon/fog world is not exported).
inline Scene testSceneWanderer() { return testSceneBlenderGLTF("assets/Wanderer/Wanderer.gltf", ""); }

// Blender EEVEE demo "Temple": stone ruins with instanced, alpha-cut grass under point and
// spot lights, under Kiran's sky HDRI (the original environment image is missing from the .blend).
inline Scene testSceneTemple() { return testSceneBlenderGLTF("assets/Temple/Temple.gltf", "assets/sky2.exr"); }

// Blender 2.91 splash "Red Autumn Forest" (Robin Tran), exported to glTF: stylized hills,
// instanced props and clouds under a sun and a spot. Its hair-particle vegetation does not
// export.
inline Scene testSceneAutumn() { return testSceneBlenderGLTF("assets/Autumn/Autumn.gltf", "assets/sky2.exr"); }

// Barcelona Pavilion (Blender Cycles demo, Hamza Cheggour / eMirage, CC-BY): sunset light on
// marble, glass and water with ~20k instanced plants. Old Cycles node setups were rebuilt as
// Principled BSDFs before the glTF export. Lit by the file's own sky photo as environment.
inline Scene testScenePavilion() { return testSceneBlenderGLTF("assets/Pavilion/Pavilion.gltf", "assets/blend_demos/pavilion/3d/textures/DSC_8129.JPG"); }

// Symmetric biconvex lens (optical axis +y) centred at the origin: two spherical caps of
// curvature radius `curvature` over an aperture of radius `aperture`. With ior n the thin-lens
// focal length is curvature / (2 (n - 1)). Triangles are oriented outward.
inline std::vector<Triangle> makeBiconvexLens(float curvature, float aperture, int matIndex, int rings = 24, int segments = 64) {
	using namespace glm;
	const float sag = curvature - std::sqrt(curvature * curvature - aperture * aperture);
	std::vector<Triangle> tris;
	auto capPoint = [&](float r, float phi, float side) {
		float y = side * (std::sqrt(curvature * curvature - r * r) - (curvature - sag));
		return vec3(r * std::cos(phi), y, r * std::sin(phi));
	};
	auto capNormal = [&](const vec3& p, float side) {
		return normalize(p - vec3(0.0f, -side * (curvature - sag), 0.0f)); // from the sphere centre
	};
	for (float side : { 1.0f, -1.0f }) {
		for (int i = 0; i < rings; ++i) {
			float r0 = aperture * i / rings, r1 = aperture * (i + 1) / rings;
			for (int j = 0; j < segments; ++j) {
				float p0 = 2.0f * pi<float>() * j / segments, p1 = 2.0f * pi<float>() * (j + 1) / segments;
				vec3 a = capPoint(r0, p0, side), b = capPoint(r1, p0, side), c = capPoint(r1, p1, side), d = capPoint(r0, p1, side);
				auto push = [&](vec3 x, vec3 y, vec3 z) {
					if (dot(cross(y - x, z - x), (x + y + z) / 3.0f) < 0.0f)
						std::swap(y, z); // keep outward (counter-clockwise from outside)
					Triangle t{};
					t.v0 = x; t.v1 = y; t.v2 = z;
					t.n0 = capNormal(x, side); t.n1 = capNormal(y, side); t.n2 = capNormal(z, side);
					t.hasNormal = true;
					t.matIndex = matIndex;
					tris.push_back(t);
				};
				if (i == 0)
					push(a, b, c); // a == d at the centre
				else {
					push(a, b, c);
					push(a, c, d);
				}
			}
		}
	}
	return tris;
}

// "Fogbow": a lens spotlight (3 mm sphere light at the focus of a glass condenser lens, inside
// a black housing with a round aperture) sends a ~10 cm collimated beam onto an upright
// brilliant floating in thin fog; the stone splits it into coloured shafts that light tracing
// renders in the haze. Dark floor, black sky, level side camera. `eta` / `abbe`: the stone
// (diamond 2.417 / 55.3, moissanite 2.65 / 12.7).
// Lens spotlight: a small sphere light at the focus f of a glass biconvex lens (n = 1.5,
// curvature = f) inside a black housing with a round aperture, so only a nearly collimated
// beam (~2 * aperture wide, divergence ~ lr / f) leaves, towards -toLight from lensC.
inline void addLensSpotlight(Scene& scene, const glm::vec3& lensC, const glm::vec3& toLight, float f, float aperture,
	float lr, float intensity, int blackMat) {
	using namespace glm;
	Material lensGlass{};
	lensGlass.albedo = vec3(1.0f);
	lensGlass.transmission = 1.0f;
	lensGlass.eta = 1.5f;
	scene.mats.push_back(lensGlass);
	const int lensMat = static_cast<int>(scene.mats.size()) - 1;
	const vec3 emitter = lensC + f * toLight;
	vec3 axis = cross(vec3(0.0f, 1.0f, 0.0f), toLight);
	mat4 orient = length(axis) < 1e-6f ? mat4(1.0f) : rotate(mat4(1.0f), std::acos(clamp(toLight.y, -1.0f, 1.0f)), normalize(axis));
	scene.addInstance(scene.addMesh(makeBiconvexLens(f, aperture, lensMat)), translate(mat4(1.0f), lensC) * orient);
	scene.addSphereLight(emitter, lr, vec3(intensity / (pi<float>() * lr * lr)));

	const vec3 a = normalize(cross(toLight, std::abs(toLight.y) < 0.9f ? vec3(0, 1, 0) : vec3(1, 0, 0)));
	const vec3 b = cross(toLight, a);
	const float half = aperture * 1.3f;
	std::vector<Triangle> housing;
	auto tri = [&](vec3 p0, vec3 p1, vec3 p2) {
		Triangle t{};
		t.v0 = p0; t.v1 = p1; t.v2 = p2;
		t.matIndex = blackMat;
		housing.push_back(t);
	};
	auto quad = [&](vec3 p0, vec3 p1, vec3 p2, vec3 p3) { tri(p0, p1, p2); tri(p0, p2, p3); };
	const float sag = f - std::sqrt(f * f - aperture * aperture);
	const vec3 back = emitter + 0.03f * toLight, front = lensC + (sag + 0.003f) * toLight; // just behind the lens surface
	vec3 cb[4], cf[4];
	for (int k = 0; k < 4; ++k) {
		vec2 q = vec2(k == 0 || k == 3 ? -half : half, k < 2 ? -half : half);
		cb[k] = back + q.x * a + q.y * b;
		cf[k] = front + q.x * a + q.y * b;
	}
	for (int k = 0; k < 4; ++k)
		quad(cb[k], cb[(k + 1) % 4], cf[(k + 1) % 4], cf[k]); // tube walls
	quad(cb[0], cb[1], cb[2], cb[3]);						  // back cap
	const int seg = 48;
	for (int k = 0; k < seg; ++k) { // aperture plate: circle (lens rim) to the tube's square
		float t0 = 2.0f * pi<float>() * k / seg, t1 = 2.0f * pi<float>() * (k + 1) / seg;
		auto rim = [&](float t) { return front + aperture * (std::cos(t) * a + std::sin(t) * b); };
		auto edge = [&](float t) {
			vec2 d(std::cos(t), std::sin(t));
			d *= half / std::max(std::abs(d.x), std::abs(d.y));
			return front + d.x * a + d.y * b;
		};
		quad(rim(t0), edge(t0), edge(t1), rim(t1));
	}
	scene.addInstance(scene.addMesh(housing));
}

inline Scene testSceneFogbow(float eta, float abbe) {
	using namespace glm;
	Scene scene;
	auto matte = [&](vec3 albedo) {
		Material m{};
		m.albedo = albedo;
		m.roughness = 1.0f;
		scene.mats.push_back(m);
		return static_cast<int>(scene.mats.size()) - 1;
	};
	const int floorMat = matte(vec3(0.05f)), black = matte(vec3(0.0f)), wallMat = matte(vec3(0.3f));
	scene.addInstance(scene.addMesh(makeFloor(vec3(0.0f), 4.0f, floorMat)));
	// A far-away grey room (8 x 8 x 4 m) so the stone has surroundings to reflect.
	{
		const float W = 4.0f, Hh = 4.0f;
		std::vector<Triangle> room;
		auto wall = [&](vec3 p0, vec3 p1, vec3 p2, vec3 p3) {
			Triangle t0{}, t1{};
			t0.v0 = p0; t0.v1 = p1; t0.v2 = p2;
			t1.v0 = p0; t1.v1 = p2; t1.v2 = p3;
			t0.matIndex = t1.matIndex = wallMat;
			room.push_back(t0);
			room.push_back(t1);
		};
		wall({ -W, 0, W }, { W, 0, W }, { W, Hh, W }, { -W, Hh, W });		 // back
		wall({ -W, 0, -W }, { -W, Hh, -W }, { W, Hh, -W }, { W, 0, -W });	 // front (behind the camera)
		wall({ -W, 0, -W }, { -W, 0, W }, { -W, Hh, W }, { -W, Hh, -W });	 // left
		wall({ W, 0, -W }, { W, Hh, -W }, { W, Hh, W }, { W, 0, W });		 // right
		wall({ -W, Hh, -W }, { -W, Hh, W }, { W, Hh, W }, { W, Hh, -W });	 // ceiling
		scene.addInstance(scene.addMesh(room));
		// Dim soft fill panel on the ceiling.
		scene.addQuadLight(vec3(-0.75f, Hh - 0.01f, -0.75f), vec3(1.5f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.5f), vec3(0.3f));
	}

	Material stone{};
	stone.albedo = vec3(1.0f);
	stone.transmission = 1.0f;
	stone.eta = eta;
	stone.dispersion = 20.0f / abbe;
	scene.mats.push_back(stone);
	const float r = 0.2f; // 40 cm across
	const vec3 stoneC(0.0f, 0.65f, 0.0f); // girdle centre (the pavilion reaches 0.86 r below)
	scene.addInstance(scene.addMesh(makeDiamond(r, static_cast<int>(scene.mats.size()) - 1)),
		translate(mat4(1.0f), stoneC));

	// Lens spotlight aimed at the crown, 0.8 m away.
	const vec3 crown = stoneC + vec3(0.0f, 0.3f * r, 0.0f); // table centre
	const vec3 toLight = normalize(vec3(-0.35f, 1.0f, -0.3f));
	addLensSpotlight(scene, crown + 0.8f * toLight, toLight, 0.15f, 0.05f, 0.003f, 1.2f, black);

	// Thin, slightly forward-scattering haze around everything.
	scene.fog = { true, vec3(-1.5f, 0.0f, -1.5f), vec3(1.5f, 1.9f, 1.5f), vec3(0.25f), vec3(0.005f), 0.3f };
	scene.hdr = gradientSky(0.0f, 0.0f);
	viewFrom(scene, vec3(0.0f, 0.75f, -2.4f), vec3(0.0f, 0.75f, 0.0f)); // level: keeps the fog box axis-aligned
	return scene;
}

// Rainbow in fog: a lens spotlight sends a strong ~5 cm collimated beam into a dense-flint prism
// at minimum deviation. The setup is built with a vertical prism and then tipped on its side,
// so the dispersed fan spreads vertically and rises to a white card; haze fills the whole space,
// and the camera looks straight at the fan.
inline Scene testSceneRainbowFog() {
	using namespace glm;
	Scene scene;
	auto matte = [&](vec3 albedo) {
		Material m{};
		m.albedo = albedo;
		m.roughness = 1.0f;
		scene.mats.push_back(m);
		return static_cast<int>(scene.mats.size()) - 1;
	};
	const int floorMat = matte(vec3(0.04f)), black = matte(vec3(0.0f)), white = matte(vec3(0.8f));

	Material flint{}; // dense flint (SF11-like): n_d = 1.785, Abbe number 25.8
	flint.albedo = vec3(1.0f);
	flint.transmission = 1.0f;
	flint.eta = 1.785f;
	flint.dispersion = 20.0f / 25.8f;
	scene.mats.push_back(flint);
	const float side = 0.25f, height = 0.4f, R = side / std::sqrt(3.0f), beamY = 0.2f;
	scene.addInstance(scene.addMesh(makePrism(side, height, static_cast<int>(scene.mats.size()) - 1)));

	// Minimum deviation (as in the prism scene): in along dIn, out along dOut.
	const float i = std::asin(1.785f * 0.5f), ang = radians(30.0f) - i;
	const vec3 dIn(std::cos(ang), 0.0f, std::sin(ang)), dOut(std::cos(ang), 0.0f, -std::sin(ang));
	const vec3 entry(-0.25f * side, beamY, -0.25f * R), exitP(0.25f * side, beamY, -0.25f * R);
	const vec3 lampL = entry - 0.6f * dIn;
	addLensSpotlight(scene, lampL, -dIn, 0.15f, 0.025f, 0.003f, 3.0f, black);

	// White card 2 m down the fan, square to it.
	const vec3 up(0.0f, 1.0f, 0.0f), cardL = exitP + 2.0f * dOut, across = normalize(cross(up, dOut));
	{
		Triangle t0{}, t1{};
		vec3 u = 0.4f * across, v(0.0f, 0.35f, 0.0f);
		vec3 p0 = cardL - u - v * 0.5f, p1 = cardL + u - v * 0.5f, p2 = cardL + u + v * 0.5f, p3 = cardL - u + v * 0.5f;
		t0.v0 = p0; t0.v1 = p1; t0.v2 = p2;
		t1.v0 = p0; t1.v1 = p2; t1.v2 = p3;
		t0.matIndex = t1.matIndex = white;
		scene.addInstance(scene.addMesh({ t0, t1 }));
	}

	// Tip the setup on its side (local z -> world y, local y -> world -z) and lift it: the fan
	// now spreads in the vertical x-y plane, rising to the right.
	const mat4 tip = translate(mat4(1.0f), vec3(0.0f, 0.6f, 0.0f)) * rotate(mat4(1.0f), radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
	scene.transformInstances(tip);
	const vec3 exitW = vec3(tip * vec4(exitP, 1.0f)), cardW = vec3(tip * vec4(cardL, 1.0f));
	scene.addInstance(scene.addMesh(makeFloor(vec3(0.0f), 4.0f, floorMat)));

	// Haze filling the whole space (camera included), so no box edge shows anywhere.
	scene.fog = { true, vec3(-5.0f, 0.0f, -5.0f), vec3(5.0f, 4.0f, 5.0f), vec3(0.6f), vec3(0.005f), 0.3f };
	const vec3 lampW = vec3(tip * vec4(lampL, 1.0f));
	(void)lampW;
	// Fill so the set reads: a dim ceiling softbox high above (outside the haze) and a faint sky.
	scene.addQuadLight(vec3(-0.5f, 3.0f, -1.0f), vec3(3.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 2.0f), vec3(0.08f));
	scene.hdr = gradientSky(vec3(0.01f, 0.012f, 0.02f), vec3(0.002f));
	const vec3 mid = 0.5f * (exitW + cardW);
	viewFrom(scene, vec3(mid.x, mid.y, mid.z - 1.6f), mid); // looking straight at the fan's plane
	return scene;
}

// Gem rainbow in fog: a lens spotlight shines down at 45 degrees from the upper left onto the
// crown of a floating, table-up brilliant. Light entering the crown reflects inside the
// pavilion and leaves through crown facets at steep angles, dispersed (the stone's fire);
// haze fills the space from the spotlight across, so the white beam shows coming in and the
// coloured beams leaving. The table's white reflection (brilliance) shows too.
// A dim blue sky shows the stone and the background; level side camera. `eta` / `abbe`: the
// stone (diamond 2.417 / 55.3,
// moissanite 2.65 / 12.7).
inline Scene testSceneGemRainbow(float eta, float abbe) {
	using namespace glm;
	Scene scene;
	auto matte = [&](vec3 albedo) {
		Material m{};
		m.albedo = albedo;
		m.roughness = 1.0f;
		scene.mats.push_back(m);
		return static_cast<int>(scene.mats.size()) - 1;
	};
	const int floorMat = matte(vec3(0.04f)), black = matte(vec3(0.0f));
	scene.addInstance(scene.addMesh(makeFloor(vec3(0.0f), 4.0f, floorMat)));

	Material stone{};
	stone.albedo = vec3(0.65f, 0.8f, 1.0f); // light blue body tint
	stone.transmission = 1.0f;
	stone.eta = eta;
	stone.dispersion = 20.0f / abbe;
	scene.mats.push_back(stone);
	const float r = 0.35f, pavilion = radians(40.75f); // 70 cm across
	// Lying on the floor as if it fell: rolled onto a pavilion facet (its normal then points
	// straight down), lifted onto the floor, and turned so the table faces the camera.
	const mat4 pose = translate(mat4(1.0f), vec3(0.0f, r * std::sin(pavilion), 0.0f)) *
		rotate(mat4(1.0f), radians(51.5f), vec3(0.0f, 1.0f, 0.0f)) *
		rotate(mat4(1.0f), -pavilion, vec3(0.0f, 0.0f, 1.0f));
	scene.addInstance(scene.addMesh(makeDiamond(r, static_cast<int>(scene.mats.size()) - 1)), pose);

	// Lens spotlight 0.8 m up-left of the table centre, 45 degrees down onto the table.
	const vec3 table = vec3(pose * vec4(0.0f, 0.32f * r, 0.0f, 1.0f)), toLight = normalize(vec3(-1.0f, 1.0f, 0.0f));
	addLensSpotlight(scene, table + 0.8f * toLight, toLight, 0.15f, 0.04f, 0.003f, 3.0f, black);

	// Haze from the spotlight across past the stone: the white beam comes in, coloured beams leave.
	const vec3 lamp = table + 0.8f * toLight;
	scene.fog = { true, vec3(lamp.x - 0.1f, 0.0f, -1.0f), vec3(1.2f, 2.2f, 1.6f), vec3(0.4f), vec3(0.005f), 0.3f };
	// One light-grey diffuse wall beside the stone (it catches the dispersed beams), a dim
	// emitter wall behind the camera facing the stone, and a ceiling softbox.
	{
		const int wallMat = matte(vec3(0.6f));
		const float x0 = -1.8f, x1 = 3.0f, z0 = -3.8f, z1 = 2.0f, H = 3.0f;
		std::vector<Triangle> walls;
		auto wall = [&](vec3 p0, vec3 p1, vec3 p2, vec3 p3) {
			Triangle t0{}, t1{};
			t0.v0 = p0; t0.v1 = p1; t0.v2 = p2;
			t1.v0 = p0; t1.v1 = p2; t1.v2 = p3;
			t0.matIndex = t1.matIndex = wallMat;
			walls.push_back(t0);
			walls.push_back(t1);
		};
		wall({ x0, 0, z0 }, { x0, 0, z1 }, { x0, H, z1 }, { x0, H, z0 }); // the one wall (on the right in view)
		scene.addInstance(scene.addMesh(walls));
		scene.addQuadLight(vec3(-2.0f + 0.6f, 0.0f, -3.6f), vec3(4.0f, 0.0f, 0.0f), vec3(0.0f, 2.5f, 0.0f), vec3(0.05f));
		// Ceiling softbox above the stone.
		scene.addQuadLight(vec3(-0.6f, H - 0.01f, -0.6f), vec3(1.2f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.2f), vec3(0.4f));
	}
	// Dim blue night sky: a not-quite-black background the stone's facets reflect.
	scene.hdr = gradientSky(vec3(0.06f, 0.09f, 0.18f), vec3(0.012f, 0.015f, 0.027f));
	// Low three-quarter view from the side away from the spotlight, outside the haze, onto the
	// stone's table; the fire sprays out across the floor and walls.
	viewFrom(scene, vec3(1.6f, 0.6f, -1.8f), vec3(0.0f, 0.3f, 0.2f));
	return scene;
}

// Blue diamond in a Cornell box: a light-blue brilliant lying fallen on the floor, table to the
// camera, under a bright ceiling light; a small lens spotlight on the table throws the stone's
// fire onto the walls. No fog.
inline Scene testSceneDiamondBox() {
	using namespace glm;
	Scene scene;
	auto matte = [&](vec3 albedo) {
		Material m{};
		m.albedo = albedo;
		m.roughness = 1.0f;
		scene.mats.push_back(m);
		return static_cast<int>(scene.mats.size()) - 1;
	};
	const int white = matte(vec3(0.75f)), red = matte(vec3(0.63f, 0.065f, 0.05f)), green = matte(vec3(0.14f, 0.45f, 0.09f)),
		black = matte(vec3(0.0f));
	const float h = 0.6f, H = 1.2f;
	std::vector<Triangle> box;
	auto quad = [&](vec3 a, vec3 b, vec3 c, vec3 d, int mat) {
		Triangle t0{}, t1{};
		t0.v0 = a; t0.v1 = b; t0.v2 = c;
		t1.v0 = a; t1.v1 = c; t1.v2 = d;
		t0.matIndex = t1.matIndex = mat;
		box.push_back(t0);
		box.push_back(t1);
	};
	quad({ -h, 0, -h }, { h, 0, -h }, { h, 0, h }, { -h, 0, h }, white); // floor
	quad({ -h, H, -h }, { -h, H, h }, { h, H, h }, { h, H, -h }, white); // ceiling
	quad({ -h, 0, h }, { h, 0, h }, { h, H, h }, { -h, H, h }, white);	 // back
	quad({ -h, 0, -h }, { -h, 0, h }, { -h, H, h }, { -h, H, -h }, red);  // left
	quad({ h, 0, -h }, { h, H, -h }, { h, H, h }, { h, 0, h }, green);	 // right
	scene.addInstance(scene.addMesh(box));

	Material stone{}; // diamond with a light blue body tint
	stone.albedo = vec3(0.65f, 0.8f, 1.0f);
	stone.transmission = 1.0f;
	stone.eta = 2.417f;
	stone.dispersion = 20.0f / 55.3f;
	scene.mats.push_back(stone);
	const float r = 0.15f, pavilion = radians(40.75f);
	// Fallen on the floor: elongated 1.5x across the roll axis (oval brilliant, long axis running
	// away from the camera), rolled onto a pavilion main facet (its slope after the stretch) so the
	// culet and girdle both touch (how it comes to rest), then turned so the back faces 30 degrees
	// off the camera; lowered onto the floor.
	const float width = 1.5f, length = 1.0f, rest = std::atan(std::tan(pavilion) / width);
	const mat4 orient = rotate(mat4(1.0f), radians(240.0f), vec3(0.0f, 1.0f, 0.0f)) *
		rotate(mat4(1.0f), -rest, vec3(0.0f, 0.0f, 1.0f)) * scale(mat4(1.0f), vec3(width, 1.0f, length));
	std::vector<Triangle> gem = makeDiamond(r, static_cast<int>(scene.mats.size()) - 1);
	float lowest = 1e30f;
	for (const Triangle& t : gem)
		for (const vec3& v : { t.v0, t.v1, t.v2 })
			lowest = std::min(lowest, (orient * vec4(v, 1.0f)).y);
	const mat4 pose = translate(mat4(1.0f), vec3(0.0f, -lowest, 0.05f)) * orient;
	scene.addInstance(scene.addMesh(gem), pose);

	// Bright ceiling light, and a small lens spotlight on the table for the fire.
	scene.addQuadLight(vec3(-0.15f, H - 0.001f, -0.15f), vec3(0.3f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.3f), vec3(10.0f));
	const vec3 table = vec3(pose * vec4(0.0f, 0.32f * r, 0.0f, 1.0f)), toLight = normalize(vec3(-1.0f, 1.0f, -0.5f));
	addLensSpotlight(scene, table + 0.5f * toLight, toLight, 0.1f, 0.03f, 0.002f, 1.0f, black);

	scene.hdr = gradientSky(0.0f, 0.0f);
	viewFrom(scene, vec3(0.0f, 0.45f, -0.95f), vec3(0.0f, 0.15f, 0.05f)); // close on the stone
	return scene;
}


// A collimated beam (long black housing) through a biconvex lens in fog: the light converges
// to a visible focal point in mid-air and diverges below it (an hourglass of light).
inline Scene testSceneLens() {
	using namespace glm;
	Scene scene;
	Material wall{};
	wall.albedo = vec3(0.15f);
	wall.roughness = 1.0f;
	scene.mats.push_back(wall);
	const vec3 lo(-2.0f, -1.5f, -1.8f), hi(2.0f, 2.6f, 3.2f);
	auto quad = [&](vec3 a, vec3 b, vec3 c, vec3 d, int mat) {
		Triangle t0{}, t1{};
		t0.v0 = a; t0.v1 = b; t0.v2 = c;
		t1.v0 = a; t1.v1 = c; t1.v2 = d;
		t0.matIndex = t1.matIndex = mat;
		return std::vector<Triangle>{ t0, t1 };
	};
	std::vector<Triangle> room;
	auto add = [&](std::vector<Triangle> q) { room.insert(room.end(), q.begin(), q.end()); };
	add(quad({ lo.x, lo.y, lo.z }, { hi.x, lo.y, lo.z }, { hi.x, lo.y, hi.z }, { lo.x, lo.y, hi.z }, 0));
	add(quad({ lo.x, hi.y, lo.z }, { lo.x, hi.y, hi.z }, { hi.x, hi.y, hi.z }, { hi.x, hi.y, lo.z }, 0));
	add(quad({ lo.x, lo.y, hi.z }, { hi.x, lo.y, hi.z }, { hi.x, hi.y, hi.z }, { lo.x, hi.y, hi.z }, 0));
	add(quad({ lo.x, lo.y, lo.z }, { lo.x, hi.y, lo.z }, { hi.x, hi.y, lo.z }, { hi.x, lo.y, lo.z }, 0));
	add(quad({ lo.x, lo.y, lo.z }, { lo.x, lo.y, hi.z }, { lo.x, hi.y, hi.z }, { lo.x, hi.y, lo.z }, 0));
	add(quad({ hi.x, lo.y, lo.z }, { hi.x, hi.y, lo.z }, { hi.x, hi.y, hi.z }, { hi.x, lo.y, hi.z }, 0));
	scene.addInstance(scene.addMesh(room));

	// Long black housing so the light leaves nearly collimated.
	Material black{};
	black.albedo = vec3(0.0f);
	black.roughness = 1.0f;
	scene.mats.push_back(black);
	const vec3 axis(0.0f, 0.0f, 1.4f);
	const float half = 0.18f, housingBottom = hi.y - 1.2f;
	std::vector<Triangle> housing;
	vec3 c[4] = { { axis.x - half, housingBottom, axis.z - half }, { axis.x + half, housingBottom, axis.z - half },
				  { axis.x + half, housingBottom, axis.z + half }, { axis.x - half, housingBottom, axis.z + half } };
	for (int k = 0; k < 4; ++k) {
		vec3 a = c[k], b = c[(k + 1) % 4];
		auto q = quad(a, b, vec3(b.x, hi.y, b.z), vec3(a.x, hi.y, a.z), 1);
		housing.insert(housing.end(), q.begin(), q.end());
	}
	scene.addInstance(scene.addMesh(housing));
	scene.addQuadLight(vec3(axis.x - half + 0.02f, hi.y - 0.05f, axis.z - half + 0.02f),
		vec3(2.0f * half - 0.04f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 2.0f * half - 0.04f), vec3(10.0f));

	// Lens: f = curvature / (2 (1.5 - 1)) = 1 m, so the beam focuses 1 m below it.
	Material glass{};
	glass.albedo = vec3(1.0f);
	glass.transmission = 1.0f;
	glass.eta = 1.5f;
	scene.mats.push_back(glass);
	scene.addInstance(scene.addMesh(makeBiconvexLens(1.0f, 0.3f, static_cast<int>(scene.mats.size()) - 1)),
		translate(mat4(1.0f), vec3(axis.x, 0.9f, axis.z)));

	scene.fog = { true, lo, hi, vec3(0.15f), vec3(0.005f), 0.4f };
	scene.hdr = HDRI{ 4, 2, std::vector<vec4>(8, vec4(0.0f)) };
	return scene;
}

// Caustic split check: a diffuse wall (z = 3) lit by an area light partly through a glass slab
// placed off to the side, so the camera sees the wall directly (not through glass). Light
// tracing on / off (--no-caustics) must agree at the centre pixel.
inline Scene testSceneCaustic() {
	using namespace glm;
	Scene scene = testSceneLights("none");
	Material glass{};
	glass.albedo = vec3(1.0f);
	glass.transmission = 1.0f;
	glass.eta = 1.5f;
	scene.mats.push_back(glass);
	int slab = scene.addMesh(makeCube(vec3(0.0f), 1.0f, static_cast<int>(scene.mats.size()) - 1));
	scene.addInstance(slab, translate(mat4(1.0f), vec3(1.15f, 0.0f, 2.0f)) * scale(mat4(1.0f), vec3(1.7f, 2.0f, 0.4f)));
	scene.addQuadLight(vec3(0.4f, -0.5f, 1.2f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f));
	return scene;
}

// Photon-merging check: the caustic scene seen through a glass block in front of the camera,
// so the probe's first diffuse vertex (the wall) is reached through glass. Merging on, merging
// off (camera paths only) and light tracing off must agree at the centre.
inline Scene testSceneCausticThrough() {
	using namespace glm;
	Scene scene = testSceneCaustic();
	int glass = static_cast<int>(scene.mats.size()) - 1; // the slab's glass (last material added before the light)
	for (int m = 0; m < static_cast<int>(scene.mats.size()); ++m)
		if (scene.mats[m].transmission > 0.0f)
			glass = m;
	scene.addInstance(scene.addMesh(makeCube(vec3(0.0f), 1.0f, glass)),
		translate(mat4(1.0f), vec3(0.0f, 0.0f, 0.6f)) * scale(mat4(1.0f), vec3(0.6f, 0.6f, 0.2f)));
	return scene;
}

