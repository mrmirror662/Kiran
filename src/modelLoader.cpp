#include "modelLoader.h"
#define TINYOBJLOADER_IMPLEMENTATION

#include <OBJ_Loader/tiny_obj_loader.h>
#include <filesystem>
#include <iostream>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <functional>
namespace fs = std::filesystem;

inline glm::vec3 objtoglm3(const float* v)
{
	return glm::vec3(v[0], v[1], v[2]);
}

inline glm::vec2 objtoglm2(const float* v)
{
	return glm::vec2(v[0], v[1]);
}

inline std::string getDirectory(const std::string& path)
{
	return fs::path(path).parent_path().string();
}

std::tuple<std::vector<Triangle>, std::vector<Material>, std::vector<Image>>
loadFromObjWithMaterials(const std::string& meshPath)
{
	std::vector<Triangle> triangles;
	std::vector<Material> materials;
	std::vector<Image> colorMaps;

	std::unordered_map<std::string, uint32_t> textureToIndex;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> tinyMaterials;
	std::string warn, err;

	std::string baseDir = getDirectory(meshPath);

	bool ok = tinyobj::LoadObj(&attrib, &shapes, &tinyMaterials, &warn, &err,
		meshPath.c_str(), baseDir.c_str(), true);

	if (!warn.empty())
		std::cout << "WARN: " << warn << std::endl;
	if (!err.empty())
		std::cerr << "ERR: " << err << std::endl;
	if (!ok)
		return {};

	// Convert materials
	for (const auto& mat : tinyMaterials)
	{
		Material m{};

		// 1. Albedo (base color)
		// Prefer map_Kd (diffuse_texname), otherwise use mat.diffuse
		m.albedo = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);

		// 2. Roughness
		// Prefer explicit roughness if available
		if (mat.roughness > 0.0f)
		{
			m.roughness = glm::clamp(mat.roughness, 0.05f, 1.0f);
		}
		else
		{
			// Improved shininess → roughness mapping
			float ns_norm = glm::clamp(mat.shininess / 1000.0f, 0.0f, 1.0f);
			m.roughness = glm::clamp(glm::sqrt(1.0f - ns_norm), 0.05f, 1.0f);
		}

		// 3. Metallic
		// Prefer explicit metallic value
		if (mat.metallic > 0.0f || !mat.metallic_texname.empty())
		{
			m.metallic = glm::clamp(mat.metallic, 0.0f, 1.0f);
		}
		else
		{
			// Heuristic fallback: strong specular & weak ambient → metallic
			float ksStrength = glm::length(glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]));
			float kaStrength = glm::length(glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]));
			m.metallic = (ksStrength > 0.5f && kaStrength < 0.1f) ? 1.0f : 0.0f;
		}

		// 4. Emission
		// Prefer explicit emission if illum model supports emissive
		if (mat.illum == 4 || mat.illum == 6 || mat.illum == 7)
		{
			glm::vec3 emissive = glm::vec3(mat.emission[0], mat.emission[1], mat.emission[2]);
			m.emission_power = glm::length(emissive);
			m.emission_color = (m.emission_power > 0.0001f) ? emissive : glm::vec3(0.0f);
		}
		else
		{
			m.emission_color = glm::vec3(0.0f);
			m.emission_power = 0.0f;
		}
		if (m.metallic >= 0.9f && glm::length(m.albedo) < 0.01f)
		{
			// Default reflectance for silver-like mirror
			m.albedo = glm::vec3(0.95f);
		}
		// 5. Eta (IOR)
		// Used for dielectric materials in your shader
		if (mat.ior >= 1.0f && mat.ior <= 2.5f && m.metallic < 0.5f)
		{
			// m.eta = mat.ior;
		}
		else
		{
			m.eta = 0.0f; // skip dielectric logic for metals
		}
		std::cout << "Material: " << mat.name << "\n";
		std::cout << "  Albedo      : (" << m.albedo.r << ", " << m.albedo.g << ", " << m.albedo.b << ")\n";
		std::cout << "  Roughness   : " << m.roughness << "\n";
		std::cout << "  Metallic    : " << m.metallic << "\n";
		std::cout << "  EmissionCol : (" << m.emission_color.r << ", " << m.emission_color.g << ", " << m.emission_color.b << ")\n";
		std::cout << "  EmissionPow : " << m.emission_power << "\n";
		std::cout << "  Eta         : " << m.eta << "\n\n";

		materials.push_back(m);

		if (!mat.diffuse_texname.empty())
		{
			std::string texPath = (fs::path(baseDir) / mat.diffuse_texname).string();
			if (textureToIndex.find(texPath) == textureToIndex.end())
			{
				Image img = imgutl::LoadImageFromPath(texPath);
				if (img.channel == 1)
				{
					std::cerr << "Skipping single-channel texture: " << texPath << std::endl;
					continue; // Skip adding this texture
				}
				colorMaps.push_back(img);
				textureToIndex[texPath] = static_cast<uint32_t>(colorMaps.size() - 1);
			}
		}
	}

	for (const auto& shape : shapes)
	{
		const auto& mesh = shape.mesh;

		for (size_t i = 0; i < mesh.indices.size(); i += 3)
		{
			auto i0 = mesh.indices[i + 0];
			auto i1 = mesh.indices[i + 1];
			auto i2 = mesh.indices[i + 2];

			if (i0.vertex_index == i1.vertex_index || i1.vertex_index == i2.vertex_index || i0.vertex_index == i2.vertex_index)
				continue;

			glm::vec3 p1 = glm::vec3(attrib.vertices[3 * i0.vertex_index + 0], attrib.vertices[3 * i0.vertex_index + 1], attrib.vertices[3 * i0.vertex_index + 2]);
			glm::vec3 p2 = glm::vec3(attrib.vertices[3 * i1.vertex_index + 0], attrib.vertices[3 * i1.vertex_index + 1], attrib.vertices[3 * i1.vertex_index + 2]);
			glm::vec3 p3 = glm::vec3(attrib.vertices[3 * i2.vertex_index + 0], attrib.vertices[3 * i2.vertex_index + 1], attrib.vertices[3 * i2.vertex_index + 2]);

			glm::vec3 n1(0), n2(0), n3(0);
			bool hasNormals = !attrib.normals.empty();
			if (hasNormals)
			{
				n1 = glm::vec3(attrib.normals[3 * i0.normal_index + 0], attrib.normals[3 * i0.normal_index + 1], attrib.normals[3 * i0.normal_index + 2]);
				n2 = glm::vec3(attrib.normals[3 * i1.normal_index + 0], attrib.normals[3 * i1.normal_index + 1], attrib.normals[3 * i1.normal_index + 2]);
				n3 = glm::vec3(attrib.normals[3 * i2.normal_index + 0], attrib.normals[3 * i2.normal_index + 1], attrib.normals[3 * i2.normal_index + 2]);
			}

			glm::vec2 uv1(0), uv2(0), uv3(0);
			uint32_t hasTex = 0;
			uint32_t colorMapIndex = 0;

			int matIndex = mesh.material_ids[i / 3];
			if (matIndex >= 0 && matIndex < tinyMaterials.size())
			{
				const auto& mat = tinyMaterials[matIndex];

				if (!mat.diffuse_texname.empty() && !attrib.texcoords.empty())
				{
					std::string texPath = (fs::path(baseDir) / mat.diffuse_texname).string();
					auto it = textureToIndex.find(texPath);
					if (it != textureToIndex.end())
					{
						colorMapIndex = it->second;
						hasTex = 1;

						uv1 = glm::vec2(attrib.texcoords[2 * i0.texcoord_index + 0], attrib.texcoords[2 * i0.texcoord_index + 1]);
						uv2 = glm::vec2(attrib.texcoords[2 * i1.texcoord_index + 0], attrib.texcoords[2 * i1.texcoord_index + 1]);
						uv3 = glm::vec2(attrib.texcoords[2 * i2.texcoord_index + 0], attrib.texcoords[2 * i2.texcoord_index + 1]);
					}
				}
			}

			Triangle t = {
				p1, p2, p3,
				matIndex,
				n1, n2, n3,
				uv1, uv2, uv3,
				hasNormals,
				hasTex,
				colorMapIndex };
			triangles.push_back(t);
		}
	}

	return { triangles, materials, colorMaps };
}
namespace
{
	// Typed, stride-aware accessor reads.
	const uint8_t* accessorElement(const tinygltf::Model& model, const tinygltf::Accessor& acc, size_t i, size_t elemSize)
	{
		const auto& view = model.bufferViews[acc.bufferView];
		size_t stride = view.byteStride ? view.byteStride : elemSize;
		return model.buffers[view.buffer].data.data() + view.byteOffset + acc.byteOffset + i * stride;
	}

	template <int N>
	glm::vec<N, float> readVec(const tinygltf::Model& model, const tinygltf::Accessor& acc, size_t i)
	{
		glm::vec<N, float> v;
		std::memcpy(&v, accessorElement(model, acc, i, sizeof(v)), sizeof(v));
		return v;
	}

	uint32_t readIndex(const tinygltf::Model& model, const tinygltf::Accessor& acc, size_t i)
	{
		switch (acc.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return *accessorElement(model, acc, i, 1);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: { uint16_t v; std::memcpy(&v, accessorElement(model, acc, i, 2), 2); return v; }
		default: { uint32_t v; std::memcpy(&v, accessorElement(model, acc, i, 4), 4); return v; }
		}
	}

	const tinygltf::Accessor* findAttribute(const tinygltf::Model& model, const std::map<std::string, int>& attrs, const char* name)
	{
		auto it = attrs.find(name);
		return it == attrs.end() ? nullptr : &model.accessors[it->second];
	}

	glm::mat4 composeTRS(const glm::vec3& t, const glm::quat& r, const glm::vec3& s)
	{
		return glm::translate(glm::mat4(1.0f), t) * glm::mat4_cast(r) * glm::scale(glm::mat4(1.0f), s);
	}

	glm::mat4 nodeLocalTransform(const tinygltf::Node& node)
	{
		if (node.matrix.size() == 16)
		{
			glm::mat4 m;
			for (int i = 0; i < 16; ++i)
				glm::value_ptr(m)[i] = static_cast<float>(node.matrix[i]);
			return m;
		}
		glm::vec3 t(0.0f), s(1.0f);
		glm::quat r(1.0f, 0.0f, 0.0f, 0.0f);
		if (node.translation.size() == 3) t = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
		if (node.rotation.size() == 4) r = glm::quat(float(node.rotation[3]), float(node.rotation[0]), float(node.rotation[1]), float(node.rotation[2]));
		if (node.scale.size() == 3) s = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
		return composeTRS(t, r, s);
	}

	Image toRGB(const Image& input)
	{
		Image output{ {}, 3, input.width, input.height };
		output.buffer.resize(size_t(input.width) * input.height * 3);
		for (int i = 0; i < input.width * input.height; ++i)
			output.buffer[i * 3 + 0] = output.buffer[i * 3 + 1] = output.buffer[i * 3 + 2] = input.buffer[i * input.channel];
		return output;
	}
}

bool loadGLTF(const std::string& path, Scene& scene, const glm::mat4& rootTransform)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;
	bool binary = fs::path(path).extension() == ".glb";
	bool ok = binary ? loader.LoadBinaryFromFile(&model, &err, &warn, path)
					 : loader.LoadASCIIFromFile(&model, &err, &warn, path);
	if (!warn.empty()) std::cout << "GLTF WARN: " << warn << "\n";
	if (!err.empty()) std::cerr << "GLTF ERR: " << err << "\n";
	if (!ok) return false;

	// --- Textures (glTF image index -> scene color map index)
	std::vector<int> imageToColorMap(model.images.size(), -1);
	for (size_t i = 0; i < model.images.size(); ++i)
	{
		const auto& image = model.images[i];
		Image img;
		if (!image.uri.empty())
			img = imgutl::LoadImageFromPath((fs::path(path).parent_path() / image.uri).string());
		else
			img = Image{ image.image, image.component, image.width, image.height };
		if (img.buffer.empty())
			continue;
		if (img.channel == 1 || img.channel == 2)
			img = toRGB(img);
		imageToColorMap[i] = static_cast<int>(scene.colorMaps.size());
		scene.colorMaps.push_back(std::move(img));
	}

	auto colorMapOf = [&](int textureIndex)
		{
			if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size()))
				return -1;
			int source = model.textures[textureIndex].source;
			return source >= 0 && source < static_cast<int>(imageToColorMap.size()) ? imageToColorMap[source] : -1;
		};

	// --- Materials
	int materialBase = static_cast<int>(scene.mats.size());
	for (const auto& mat : model.materials)
	{
		const auto& pbr = mat.pbrMetallicRoughness;
		Material m{};
		m.albedo = glm::vec3(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2]);
		m.roughness = static_cast<float>(pbr.roughnessFactor);
		m.metallic = static_cast<float>(pbr.metallicFactor);
		// glTF: emitted radiance = emissiveFactor * emissiveTexture * emissive_strength
		if (mat.emissiveFactor.size() == 3)
			m.emission_color = glm::vec3(mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2]);
		m.emission_power = 1.0f;
		auto strength = mat.extensions.find("KHR_materials_emissive_strength");
		if (strength != mat.extensions.end() && strength->second.Has("emissiveStrength"))
			m.emission_power = static_cast<float>(strength->second.Get("emissiveStrength").GetNumberAsDouble());
		m.emissionMap = colorMapOf(mat.emissiveTexture.index);
		m.alphaMode = mat.alphaMode == "MASK" ? 1 : mat.alphaMode == "BLEND" ? 2 : 0;
		m.alphaCutoff = static_cast<float>(mat.alphaCutoff);

		// KHR_materials_transmission / ior / volume. Without a volume (thicknessFactor 0) the
		// surface is thin-walled: light passes straight through, tinted by the base color.
		auto ext = [&](const char* name) -> const tinygltf::Value*
			{
				auto it = mat.extensions.find(name);
				return it == mat.extensions.end() ? nullptr : &it->second;
			};
		auto number = [](const tinygltf::Value* v, const char* key, double fallback)
			{
				return v && v->Has(key) ? v->Get(key).GetNumberAsDouble() : fallback;
			};
		if (const auto* tr = ext("KHR_materials_transmission"))
		{
			m.transmission = static_cast<float>(number(tr, "transmissionFactor", 0.0));
			m.eta = static_cast<float>(number(ext("KHR_materials_ior"), "ior", 1.5));
			const auto* vol = ext("KHR_materials_volume");
			float thickness = static_cast<float>(number(vol, "thicknessFactor", 0.0));
			m.thinWalled = thickness > 0.0f ? 0 : 1;
			m.dispersion = static_cast<float>(number(ext("KHR_materials_dispersion"), "dispersion", 0.0));
			double attDist = number(vol, "attenuationDistance", 0.0);
			if (vol && vol->Has("attenuationColor") && attDist > 0.0)
			{
				const auto& c = vol->Get("attenuationColor");
				for (int k = 0; k < 3; ++k)
					m.sigma_a[k] = static_cast<float>(-std::log(std::max(c.Get(k).GetNumberAsDouble(), 1e-6)) / attDist);
			}
		}
		scene.mats.push_back(m);
	}
	int defaultMaterial = -1; // created on demand for primitives without a material
	auto materialFor = [&](int gltfMaterial)
		{
			if (gltfMaterial >= 0 && gltfMaterial < static_cast<int>(model.materials.size()))
				return materialBase + gltfMaterial;
			if (defaultMaterial == -1)
			{
				Material m{};
				m.albedo = glm::vec3(0.8f);
				m.roughness = 0.5f;
				defaultMaterial = static_cast<int>(scene.mats.size());
				scene.mats.push_back(m);
			}
			return defaultMaterial;
		};

	// --- Meshes: each glTF mesh (all its primitives) becomes one BLAS
	std::vector<int> meshIds;
	for (const auto& mesh : model.meshes)
	{
		std::vector<Triangle> tris;
		for (const auto& prim : mesh.primitives)
		{
			const auto* pos = findAttribute(model, prim.attributes, "POSITION");
			if (prim.mode != TINYGLTF_MODE_TRIANGLES || !pos)
				continue;
			const auto* nrm = findAttribute(model, prim.attributes, "NORMAL");
			const auto* tex = findAttribute(model, prim.attributes, "TEXCOORD_0");
			const tinygltf::Accessor* idx = prim.indices >= 0 ? &model.accessors[prim.indices] : nullptr;

			int matIndex = materialFor(prim.material);
			int colorMap = prim.material >= 0 && prim.material < static_cast<int>(model.materials.size())
				? colorMapOf(model.materials[prim.material].pbrMetallicRoughness.baseColorTexture.index) : -1;
			bool hasTex = colorMap >= 0 && tex;

			size_t count = idx ? idx->count : pos->count;
			for (size_t i = 0; i + 2 < count; i += 3)
			{
				uint32_t v[3];
				for (int k = 0; k < 3; ++k)
					v[k] = idx ? readIndex(model, *idx, i + k) : static_cast<uint32_t>(i + k);

				Triangle t{};
				t.v0 = readVec<3>(model, *pos, v[0]);
				t.v1 = readVec<3>(model, *pos, v[1]);
				t.v2 = readVec<3>(model, *pos, v[2]);
				t.matIndex = matIndex;
				if (nrm)
				{
					t.n0 = readVec<3>(model, *nrm, v[0]);
					t.n1 = readVec<3>(model, *nrm, v[1]);
					t.n2 = readVec<3>(model, *nrm, v[2]);
				}
				if (tex) // also needed by emissive textures
				{
					t.uv0 = readVec<2>(model, *tex, v[0]);
					t.uv1 = readVec<2>(model, *tex, v[1]);
					t.uv2 = readVec<2>(model, *tex, v[2]);
				}
				t.hasNormal = nrm != nullptr;
				t.hasTexture = hasTex;
				t.colorMapIndex = hasTex ? static_cast<uint32_t>(colorMap) : 0;
				tris.push_back(t);
			}
		}
		meshIds.push_back(scene.addMesh(std::move(tris)));
	}

	// --- Node hierarchy -> instances (including EXT_mesh_gpu_instancing)
	std::function<void(int, const glm::mat4&)> visit = [&](int nodeIndex, const glm::mat4& parent)
		{
			const auto& node = model.nodes[nodeIndex];
			glm::mat4 world = parent * nodeLocalTransform(node);
			if (node.camera >= 0 && !scene.camera)
			{
				scene.camera = world;
				const auto& cam = model.cameras[node.camera];
				if (cam.type == "perspective" && cam.perspective.yfov > 0.0)
					scene.cameraYFov = static_cast<float>(cam.perspective.yfov);
			}
			if (node.light >= 0 && node.light < static_cast<int>(model.lights.size()))
			{
				// KHR_lights_punctual: lights sit at the node origin and shine down its -Z.
				const auto& src = model.lights[node.light];
				Light l;
				l.type = src.type == "directional" ? LightType::Directional
					: src.type == "spot" ? LightType::Spot : LightType::Point;
				if (src.color.size() == 3)
					l.color = glm::vec3(src.color[0], src.color[1], src.color[2]);
				l.intensity = static_cast<float>(src.intensity);
				l.position = glm::vec3(world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
				l.direction = glm::normalize(glm::mat3(world) * glm::vec3(0.0f, 0.0f, -1.0f));
				l.innerCone = static_cast<float>(src.spot.innerConeAngle);
				l.outerCone = static_cast<float>(src.spot.outerConeAngle);
				// Not in KHR_lights_punctual: a sun's angular diameter (radians) in the light's
				// extras, as written by our Blender export (0 or absent: delta light).
				if (src.extras.IsObject() && src.extras.Has("angle"))
					l.angle = static_cast<float>(src.extras.Get("angle").GetNumberAsDouble());
				scene.addLight(l);
			}
			if (node.mesh >= 0)
			{
				auto ext = node.extensions.find("EXT_mesh_gpu_instancing");
				if (ext != node.extensions.end() && ext->second.Has("attributes"))
				{
					const auto& attrs = ext->second.Get("attributes");
					auto accessorOf = [&](const char* name) -> const tinygltf::Accessor*
						{
							return attrs.Has(name) ? &model.accessors[attrs.Get(name).GetNumberAsInt()] : nullptr;
						};
					const auto* T = accessorOf("TRANSLATION");
					const auto* R = accessorOf("ROTATION");
					const auto* S = accessorOf("SCALE");
					size_t n = T ? T->count : R ? R->count : S ? S->count : 0;
					for (size_t i = 0; i < n; ++i)
					{
						glm::vec3 t = T ? readVec<3>(model, *T, i) : glm::vec3(0.0f);
						glm::vec4 q = R ? readVec<4>(model, *R, i) : glm::vec4(0, 0, 0, 1);
						glm::vec3 s = S ? readVec<3>(model, *S, i) : glm::vec3(1.0f);
						scene.addInstance(meshIds[node.mesh], world * composeTRS(t, glm::quat(q.w, q.x, q.y, q.z), s));
					}
				}
				else
				{
					scene.addInstance(meshIds[node.mesh], world);
				}
			}
			for (int child : node.children)
				visit(child, world);
		};

	int sceneIndex = model.defaultScene >= 0 ? model.defaultScene : 0;
	if (sceneIndex < static_cast<int>(model.scenes.size()))
	{
		for (int root : model.scenes[sceneIndex].nodes)
			visit(root, rootTransform);
	}
	else
	{
		for (int id : meshIds)
			scene.addInstance(id, rootTransform);
	}

	std::cout << "[GLTF] " << path << ": " << meshIds.size() << " meshes, "
			  << scene.instances.size() << " instances in scene\n";
	return true;
}
