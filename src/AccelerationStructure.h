#pragma once
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <cstring>

#include "BVH.h"
#include "primitives.h"
#include <glm/glm.hpp>
#include <limits>
#include <vector>

// GPU instance record (std430, matches `Instance` in rt.comp).
struct GPUInstance
{
	glm::mat4 objectToWorld;
	glm::mat4 worldToObject;
	int blasRoot;		  // root of the mesh's BLAS in blasGpu
	int materialOverride; // -1: use triangle materials
	int lightIndexBase;	  // slice of LightTable::lightIndexOf for emissive triangles, -1: none
	int triStart;		  // first triangle of the mesh in the global triangle buffer
};
static_assert(sizeof(GPUInstance) == 144, "GPUInstance must match std430 layout");

// GPU BVH node with both child boxes inline (std430, matches `BVHNode` in rt.comp), so one
// 64-byte fetch decides where to go next. Per child: count > 0 is a leaf covering primitives
// [ref, ref + count); count == 0 makes ref an interior node index; ref == -1 is no child.
struct GPUBVHNode
{
	glm::vec3 lmin;
	int lref;
	glm::vec3 lmax;
	int rref;
	glm::vec3 rmin;
	int lcount;
	glm::vec3 rmax;
	int rcount;
};
static_assert(sizeof(GPUBVHNode) == 64, "GPUBVHNode must match std430 layout");

// Two-level acceleration structure.
//   BLAS: one BVH per mesh in object space. All meshes share one triangle buffer and one
//         node buffer; node/triangle indices are global.
//   TLAS: BVH over instance world-space bounds; leaves index into `instances`.
// Rays are transformed into object space per instance, so meshes are never duplicated.
class AccelerationStructure
{
public:
	AccelerationStructure(const Scene& scene, int maxDepth)
	{
		std::vector<int> blasRoot, meshTriStart, meshTriCount;
		std::vector<BoundingBox> meshBounds;
		for (const auto& mesh : scene.meshes)
		{
			BVH blas(mesh, maxDepth);
			int triOffset = static_cast<int>(triangles.size());
			blasRoot.push_back(flatten(blas.nodes, 0, triOffset, blasGpu));
			meshTriStart.push_back(triOffset);
			meshTriCount.push_back(static_cast<int>(blas.triangles.size()));
			triangles.insert(triangles.end(), blas.triangles.begin(), blas.triangles.end());
			meshBounds.push_back(blas.nodes[0].box);
		}

		// Positions only, packed for traversal (the full Triangle is fetched once per hit).
		// v0.w flags alpha-tested triangles for traversal: 0 opaque, > 0 MASK cutoff,
		// < 0 BLEND (stochastic coverage); both read the base color texture's alpha.
		triangleVerts.reserve(triangles.size() * 3);
		for (const auto& t : triangles)
		{
			float alpha = 0.0f;
			if (t.hasTexture && t.matIndex >= 0 && t.matIndex < static_cast<int>(scene.mats.size()))
			{
				const Material& m = scene.mats[t.matIndex];
				alpha = m.alphaMode == 1 ? std::max(m.alphaCutoff, 1e-4f) : m.alphaMode == 2 ? -1.0f : 0.0f;
			}
			triangleVerts.emplace_back(t.v0, alpha);
			triangleVerts.emplace_back(t.v1, 0.0f);
			triangleVerts.emplace_back(t.v2, 0.0f);
		}

		// Build the TLAS over (instance, bounds, triangle range) so they are reordered together.
		struct Item
		{
			GPUInstance instance;
			BoundingBox box;
			int triCount;
		};
		std::vector<Item> items;
		for (const auto& inst : scene.instances)
		{
			if (inst.mesh < 0 || inst.mesh >= static_cast<int>(scene.meshes.size()) || scene.meshes[inst.mesh].empty())
				continue;
			GPUInstance g{ inst.transform, glm::inverse(inst.transform), blasRoot[inst.mesh], inst.materialOverride, -1, meshTriStart[inst.mesh] };
			items.push_back({ g, transformBox(meshBounds[inst.mesh], inst.transform), meshTriCount[inst.mesh] });
		}
		if (!items.empty())
		{
			buildBVH(items, tlasNodes, maxDepth, 1,
				[](const Item& it) { return it.box; },
				[](const Item& it) { return (it.box.min + it.box.max) * 0.5f; });
			flatten(tlasNodes, 0, 0, tlasGpu);
		}
		for (const auto& it : items)
		{
			instances.push_back(it.instance);
			instanceTriCount.push_back(it.triCount);
		}
	}

	std::vector<Triangle> triangles;	  // all BLAS triangles, object space
	std::vector<glm::vec4> triangleVerts; // v0, v1, v2 per triangle (same order as triangles)
	std::vector<GPUBVHNode> blasGpu;	  // all BLAS nodes (GPU layout)
	std::vector<GPUBVHNode> tlasGpu;	  // TLAS nodes (GPU layout); leaves cover instances
	std::vector<BVHNode> tlasNodes;		  // TLAS (build layout); [0] holds the scene bounds
	std::vector<GPUInstance> instances;
	std::vector<int> instanceTriCount;	  // triangles in each instance's mesh

private:
	static BoundingBox transformBox(const BoundingBox& box, const glm::mat4& m)
	{
		BoundingBox out;
		for (int i = 0; i < 8; ++i)
		{
			glm::vec3 corner((i & 1) ? box.max.x : box.min.x,
				(i & 2) ? box.max.y : box.min.y,
				(i & 4) ? box.max.z : box.min.z);
			out.expand(glm::vec3(m * glm::vec4(corner, 1.0f)));
		}
		return out;
	}

	// Converts a build-layout subtree into GPU nodes; leaf primitive ranges are offset by
	// `primOffset`. A tree that is a single leaf gets a root with one child. Returns the root.
	static int flatten(const std::vector<BVHNode>& src, int nodeIndex, int primOffset, std::vector<GPUBVHNode>& dst)
	{
		auto isLeaf = [&](int i) { return src[i].left == -1 && src[i].right == -1; };
		int out = static_cast<int>(dst.size());
		dst.emplace_back();

		int children[2] = { src[nodeIndex].left, src[nodeIndex].right };
		if (isLeaf(nodeIndex))
		{
			children[0] = nodeIndex;
			children[1] = -1;
		}

		glm::vec3 mins[2], maxs[2];
		int refs[2], counts[2];
		for (int k = 0; k < 2; ++k)
		{
			int c = children[k];
			if (c == -1)
			{
				mins[k] = glm::vec3(std::numeric_limits<float>::max());
				maxs[k] = glm::vec3(std::numeric_limits<float>::lowest());
				refs[k] = -1;
				counts[k] = 0;
				continue;
			}
			mins[k] = src[c].box.min;
			maxs[k] = src[c].box.max;
			if (isLeaf(c))
			{
				refs[k] = src[c].start + primOffset;
				counts[k] = src[c].end - src[c].start;
			}
			else
			{
				refs[k] = flatten(src, c, primOffset, dst); // may reallocate dst
				counts[k] = 0;
			}
		}
		dst[out] = { mins[0], refs[0], maxs[0], refs[1], mins[1], counts[0], maxs[1], counts[1] };
		return out;
	}
};
