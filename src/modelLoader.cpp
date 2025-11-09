#include "modelLoader.h"
#define TINYOBJLOADER_IMPLEMENTATION

#include <OBJ_Loader/tiny_obj_loader.h>
#include <filesystem>
#include <iostream>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>
#include <glm/gtc/type_ptr.hpp>
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
std::tuple<std::vector<Triangle>, std::vector<Material>, std::vector<Image>>
loadFromGLTFWithMaterials(const std::string& path)
{
	std::vector<Triangle> triangles;
	std::vector<Material> materials;
	std::vector<Image> colorMaps;
	std::unordered_map<std::string, uint32_t> textureToIndex;

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;

	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
	if (!warn.empty()) std::cout << "GLTF WARN: " << warn << "\n";
	if (!err.empty()) std::cerr << "GLTF ERR: " << err << "\n";
	if (!ret) return {};

	// --- Grayscale to RGB helper
	auto convert1ChannelTo3Channel = [](const Image& input) -> Image {
		Image output;
		output.width = input.width;
		output.height = input.height;
		output.channel = 3;
		output.buffer.resize(output.width * output.height * 3);
		for (int i = 0; i < output.width * output.height; ++i)
		{
			uint8_t g = input.buffer[i];
			output.buffer[i * 3 + 0] = g;
			output.buffer[i * 3 + 1] = g;
			output.buffer[i * 3 + 2] = g;
		}
		return output;
		};

	// --- Load textures
	for (const auto& image : model.images)
	{
		std::string texPath = image.uri;
		Image img = imgutl::LoadImageFromPath((fs::path(path).parent_path() / texPath).string());

		if (img.channel == 1)
		{
			std::cout << "[GLTF] Converting grayscale to RGB: " << texPath << "\n";
			img = convert1ChannelTo3Channel(img);
		}

		colorMaps.push_back(img);
		textureToIndex[texPath] = static_cast<uint32_t>(colorMaps.size() - 1);
		std::cout << "[GLTF] Added color map: " << texPath << " at index " << (colorMaps.size() - 1) << "\n";
	}

	// --- Load materials
	for (const auto& mat : model.materials)
	{
		Material m{};
		auto& pbr = mat.pbrMetallicRoughness;

		m.albedo = glm::vec3(
			static_cast<float>(pbr.baseColorFactor[0]),
			static_cast<float>(pbr.baseColorFactor[1]),
			static_cast<float>(pbr.baseColorFactor[2]));

		m.roughness = static_cast<float>(pbr.roughnessFactor);
		m.metallic = static_cast<float>(pbr.metallicFactor);

		if (mat.emissiveFactor.size() == 3)
		{
			m.emission_color = glm::vec3(
				static_cast<float>(mat.emissiveFactor[0]),
				static_cast<float>(mat.emissiveFactor[1]),
				static_cast<float>(mat.emissiveFactor[2]));
			m.emission_power = glm::length(m.emission_color);
		}

		std::cout << "Material: " << mat.name << "\n";
		std::cout << "  Albedo    : (" << m.albedo.r << ", " << m.albedo.g << ", " << m.albedo.b << ")\n";
		std::cout << "  Roughness : " << m.roughness << "\n";
		std::cout << "  Metallic  : " << m.metallic << "\n\n";

		materials.push_back(m);
	}

	// --- Accessor buffer helper
	auto getBufferData = [&](const tinygltf::Accessor& accessor)
		{
			const auto& bufferView = model.bufferViews[accessor.bufferView];
			const auto& buffer = model.buffers[bufferView.buffer];
			const uint8_t* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
			return dataPtr;
		};

	// --- Load geometry
	for (const auto& mesh : model.meshes)
	{
		for (const auto& prim : mesh.primitives)
		{
			if (prim.mode != TINYGLTF_MODE_TRIANGLES)
				continue;

			const auto& posAcc = model.accessors.at(prim.attributes.at("POSITION"));
			const auto& normAcc = model.accessors.at(prim.attributes.at("NORMAL"));
			const auto& texAcc = prim.attributes.count("TEXCOORD_0") ? model.accessors.at(prim.attributes.at("TEXCOORD_0")) : tinygltf::Accessor{};
			const auto& idxAcc = model.accessors.at(prim.indices);

			const float* posData = reinterpret_cast<const float*>(getBufferData(posAcc));
			const float* normData = reinterpret_cast<const float*>(getBufferData(normAcc));
			const float* texData = texAcc.count > 0 ? reinterpret_cast<const float*>(getBufferData(texAcc)) : nullptr;
			const unsigned short* idxData = reinterpret_cast<const unsigned short*>(getBufferData(idxAcc));

			bool hasNormals = normAcc.count > 0;
			bool hasUVs = texAcc.count > 0;

			bool hasTex = false;
			uint32_t colorMapIndex = 0;

			// --- Assign texture per triangle
			if (prim.material >= 0 && prim.material < model.materials.size())
			{
				const auto& mat = model.materials[prim.material];
				const auto& pbr = mat.pbrMetallicRoughness;

				if (pbr.baseColorTexture.index >= 0 && pbr.baseColorTexture.index < model.textures.size())
				{
					int imgSource = model.textures[pbr.baseColorTexture.index].source;
					if (imgSource >= 0 && imgSource < model.images.size())
					{
						std::string uri = model.images[imgSource].uri;
						auto it = textureToIndex.find(uri);
						if (it != textureToIndex.end())
						{
							colorMapIndex = it->second;
							hasTex = true;
							std::cout << "[GLTF DEBUG] Using texture for material: " << mat.name << ", texture: " << uri << ", index: " << colorMapIndex << "\n";
						}
					}
				}
			}

			// --- Create triangles
			for (size_t i = 0; i < idxAcc.count; i += 3)
			{
				uint32_t i0 = idxData[i + 0];
				uint32_t i1 = idxData[i + 1];
				uint32_t i2 = idxData[i + 2];

				glm::vec3 p1 = glm::make_vec3(&posData[3 * i0]);
				glm::vec3 p2 = glm::make_vec3(&posData[3 * i1]);
				glm::vec3 p3 = glm::make_vec3(&posData[3 * i2]);

				glm::vec3 n1(0), n2(0), n3(0);
				if (hasNormals)
				{
					n1 = glm::make_vec3(&normData[3 * i0]);
					n2 = glm::make_vec3(&normData[3 * i1]);
					n3 = glm::make_vec3(&normData[3 * i2]);
				}

				glm::vec2 uv1(0), uv2(0), uv3(0);
				if (hasTex && hasUVs)
				{
					uv1 = glm::make_vec2(&texData[2 * i0]);
					uv2 = glm::make_vec2(&texData[2 * i1]);
					uv3 = glm::make_vec2(&texData[2 * i2]);
				}

				triangles.push_back({
					p1, p2, p3,
					prim.material,
					n1, n2, n3,
					uv1, uv2, uv3,
					hasNormals,
					hasTex,
					colorMapIndex
					});
			}
		}
	}

	return { triangles, materials, colorMaps };
}