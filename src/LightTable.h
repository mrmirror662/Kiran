#pragma once

#include "AccelerationStructure.h"
#include "primitives.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

// GPU light record (std430, matches `Light` in rt.comp).
struct GPULight
{
	int type;	   // LightType
	int instance;  // EmissiveTriangle: GPU instance; Sphere: sphere index
	int triangle;  // EmissiveTriangle: global triangle index
	int bitTrail;  // path from the light-BVH root to this light's leaf (bit i: 0 left, 1 right)
	glm::vec4 posA; // position / sphere center; w: sphere radius or spot cos(inner)
	glm::vec4 dirB; // direction light travels; w: spot cos(outer)
	glm::vec4 emit; // intensity / irradiance / radiance (rgb)
};
static_assert(sizeof(GPULight) == 64, "GPULight must match std430 layout");

// Light-BVH node (std430, matches `LightNode` in rt.comp). Left child is the next node;
// `childOrLight` is the right child, or the light index for a leaf.
struct GPULightNode
{
	glm::vec4 boundsMinPhi;	 // xyz: bounds min, w: total power
	glm::vec4 boundsMaxCosO; // xyz: bounds max, w: cos(theta_o) of the normal cone
	glm::vec4 axisCosE;		 // xyz: cone axis, w: cos(theta_e) emission spread
	int childOrLight;
	int flags; // bit 0: leaf, bit 1: two-sided
	int pad[2];
};
static_assert(sizeof(GPULightNode) == 64, "GPULightNode must match std430 layout");

// Light importance sampling (pbrt-v4 BVHLightSampler, after Conty Estevez & Kulla 2018).
// Bounded lights (emissive triangles, spheres, points, spots) live in a light BVH whose nodes
// carry power, spatial bounds and an orientation cone; the GPU walks it with one random number,
// choosing children by their estimated contribution to the shading point. Directional lights
// are kept separately at the front of `lights` and chosen uniformly.
class LightTable
{
public:
	LightTable(const Scene& scene, AccelerationStructure& accel)
	{
		float sceneRadius = 1.0f;
		if (!accel.tlasNodes.empty())
			sceneRadius = 0.5f * glm::length(accel.tlasNodes[0].box.max - accel.tlasNodes[0].box.min);
		(void)sceneRadius;

		// Directional (infinite) lights first.
		for (const Light& l : scene.lights)
		{
			if (l.type != LightType::Directional)
				continue;
			GPULight g{};
			g.type = static_cast<int>(l.type);
			// w: cos of the half-angle for a sun with angular size (a disc at infinity), 0: delta.
			g.dirB = glm::vec4(glm::normalize(l.direction), l.angle > 0.0f ? std::cos(0.5f * l.angle) : 0.0f);
			g.emit = glm::vec4(l.color * l.intensity, 0.0f);
			lights.push_back(g);
		}
		infiniteCount = static_cast<int>(lights.size());

		std::vector<Bounded> bounded;
		auto addBounded = [&](const GPULight& g, const Bounds& b)
			{
				if (b.phi <= 0.0f)
					return;
				bounded.push_back({ static_cast<int>(lights.size()), b });
				lights.push_back(g);
				lightPower.resize(lights.size(), 0.0f);
				lightPower.back() = b.phi;
			};

		for (const Light& l : scene.lights)
		{
			GPULight g{};
			g.type = static_cast<int>(l.type);
			glm::vec3 c = l.color * l.intensity;
			Bounds b;
			switch (l.type)
			{
			case LightType::Point:
				g.posA = glm::vec4(l.position, 0.0f);
				b = pointBounds(l.position, 4.0f * pi * luminance(c), -1.0f, glm::vec3(0, 0, 1), 0.0f);
				break;
			case LightType::Spot:
			{
				g.posA = glm::vec4(l.position, std::cos(l.innerCone));
				g.dirB = glm::vec4(glm::normalize(l.direction), std::cos(l.outerCone));
				// pbrt: normal cone = falloff start, emission spread = falloff width.
				float cosE = std::cos(std::max(0.0f, l.outerCone - l.innerCone));
				b = pointBounds(l.position, 4.0f * pi * luminance(c), std::cos(l.innerCone), glm::normalize(l.direction), cosE);
				break;
			}
			case LightType::Sphere:
			{
				if (l.sphere < 0 || l.sphere >= static_cast<int>(scene.spheres.size()))
					continue;
				const Sphere& s = scene.spheres[l.sphere];
				const Material& m = scene.mats[s.matIndex];
				c = m.emission_color * m.emission_power;
				g.instance = l.sphere;
				g.posA = glm::vec4(s.center, s.r);
				b.box.expand(s.center - glm::vec3(s.r));
				b.box.expand(s.center + glm::vec3(s.r));
				b.phi = pi * 4.0f * pi * s.r * s.r * luminance(c);
				b.axis = glm::vec3(0, 0, 1);
				b.cosO = -1.0f; // emits in every direction
				b.cosE = 0.0f;
				break;
			}
			default:
				continue;
			}
			g.emit = glm::vec4(c, 0.0f);
			addBounded(g, b);
		}

		// Emissive triangles (two-sided). Each instance with emitters gets a slice of
		// `lightIndexOf` mapping its BLAS triangles to light indices, so a BSDF hit on an
		// emitter can find its light (and bit trail) for MIS.
		for (size_t i = 0; i < accel.instances.size(); ++i)
		{
			GPUInstance& inst = accel.instances[i];
			inst.lightIndexBase = -1;
			int base = static_cast<int>(lightIndexOf.size());
			bool any = false;
			for (int t = inst.triStart; t < inst.triStart + accel.instanceTriCount[i]; ++t)
			{
				int lightIndex = -1;
				const Triangle& tri = accel.triangles[t];
				int mi = inst.materialOverride >= 0 ? inst.materialOverride : tri.matIndex;
				if (mi >= 0 && mi < static_cast<int>(scene.mats.size()))
				{
					const Material& m = scene.mats[mi];
					float lum = luminance(m.emission_color * m.emission_power);
					glm::vec3 p0 = glm::vec3(inst.objectToWorld * glm::vec4(tri.v0, 1.0f));
					glm::vec3 p1 = glm::vec3(inst.objectToWorld * glm::vec4(tri.v1, 1.0f));
					glm::vec3 p2 = glm::vec3(inst.objectToWorld * glm::vec4(tri.v2, 1.0f));
					glm::vec3 cr = glm::cross(p1 - p0, p2 - p0);
					float area = 0.5f * glm::length(cr);
					if (lum > 0.0f && area > 0.0f)
					{
						GPULight g{};
						g.type = static_cast<int>(LightType::EmissiveTriangle);
						g.instance = static_cast<int>(i);
						g.triangle = t;
						Bounds b;
						b.box.expand(p0);
						b.box.expand(p1);
						b.box.expand(p2);
						b.phi = 2.0f * pi * area * lum;
						b.axis = glm::normalize(cr);
						b.cosO = 1.0f; // a single normal direction
						b.cosE = 0.0f; // emits over the hemisphere
						b.twoSided = true;
						lightIndex = static_cast<int>(lights.size());
						addBounded(g, b);
						any = true;
					}
				}
				lightIndexOf.push_back(lightIndex);
			}
			if (any)
				inst.lightIndexBase = base;
			else
				lightIndexOf.resize(base);
		}

		if (!bounded.empty())
			build(bounded, 0, static_cast<int>(bounded.size()), 0u, 0);

		for (int i = 0; i < static_cast<int>(lights.size()); ++i)
			if (lights[i].type <= static_cast<int>(LightType::Directional) &&
				!(lights[i].type == static_cast<int>(LightType::Directional) && lights[i].dirB.w > 0.0f)) // sized suns are hittable
				deltaLights.push_back(i);
		if (deltaLights.empty())
			deltaLights.push_back(-1); // keep the GPU buffer non-empty

		// Emission CDF by power for light tracing (directional lights are not emitted).
		lightPower.resize(lights.size(), 0.0f);
		float total = 0.0f;
		for (float p : lightPower)
			emitCdf.push_back(total += p);
		for (float& c : emitCdf)
			c = total > 0.0f ? c / total : 0.0f;
		if (emitCdf.empty())
			emitCdf.push_back(0.0f);
		if (lightIndexOf.empty())
			lightIndexOf.push_back(-1); // keep the GPU buffer non-empty
		std::cout << "Light BVH: " << bounded.size() << " bounded lights, " << nodes.size()
				  << " nodes, depth " << maxDepthSeen << ", " << infiniteCount << " directional" << std::endl;
		if (maxDepthSeen > 31)
			std::cerr << "Light BVH deeper than 32 levels: MIS pmfs for the deepest lights are approximate" << std::endl;
	}

	std::vector<GPULight> lights;
	std::vector<GPULightNode> nodes;
	std::vector<int> lightIndexOf; // per-instance slices: BLAS triangle -> light index (-1: none)
	std::vector<float> emitCdf;	   // inclusive power CDF over `lights` for light tracing
	std::vector<int> deltaLights;  // point / spot / directional light indices (-1 sentinel if none)
	int deltaLightCount() const { return deltaLights[0] < 0 ? 0 : static_cast<int>(deltaLights.size()); }
	int infiniteCount = 0;

private:
	static constexpr float pi = glm::pi<float>();

	struct Bounds
	{
		BoundingBox box;
		float phi = 0.0f;
		glm::vec3 axis{ 0, 0, 1 };
		float cosO = 1.0f; // normal cone half-angle
		float cosE = 1.0f; // emission spread beyond the normal cone
		bool twoSided = false;
	};
	struct Bounded
	{
		int light;
		Bounds b;
	};

	int maxDepthSeen = 0;
	std::vector<float> lightPower;

	static float luminance(const glm::vec3& c) { return glm::dot(c, glm::vec3(0.2126f, 0.7152f, 0.0722f)); }

	static Bounds pointBounds(const glm::vec3& p, float phi, float cosO, const glm::vec3& axis, float cosE)
	{
		Bounds b;
		b.box.expand(p);
		b.box.expand(p);
		b.phi = phi;
		b.axis = axis;
		b.cosO = cosO;
		b.cosE = cosE;
		return b;
	}

	static float safeAcos(float x) { return std::acos(std::clamp(x, -1.0f, 1.0f)); }

	// Smallest cone containing both cones (pbrt DirectionCone Union).
	static void unionCone(glm::vec3& w, float& cosO, const glm::vec3& wb, float cosOb)
	{
		float ta = safeAcos(cosO), tb = safeAcos(cosOb);
		float td = safeAcos(glm::dot(w, wb));
		if (std::min(td + tb, pi) <= ta)
			return;
		if (std::min(td + ta, pi) <= tb)
		{
			w = wb;
			cosO = cosOb;
			return;
		}
		float to = 0.5f * (ta + td + tb);
		glm::vec3 wr = glm::cross(w, wb);
		if (to >= pi || glm::dot(wr, wr) == 0.0f)
		{
			cosO = -1.0f;
			return;
		}
		// Rotate w towards wb by (to - ta) around wr (Rodrigues).
		float tr = to - ta;
		glm::vec3 k = glm::normalize(wr);
		w = w * std::cos(tr) + glm::cross(k, w) * std::sin(tr) + k * glm::dot(k, w) * (1.0f - std::cos(tr));
		w = glm::normalize(w);
		cosO = std::cos(to);
	}

	static Bounds unionBounds(const Bounds& a, const Bounds& b)
	{
		if (a.phi == 0.0f) return b;
		if (b.phi == 0.0f) return a;
		Bounds r = a;
		r.box.expand(b.box);
		r.phi = a.phi + b.phi;
		unionCone(r.axis, r.cosO, b.axis, b.cosO);
		r.cosE = std::min(a.cosE, b.cosE);
		r.twoSided = a.twoSided || b.twoSided;
		return r;
	}

	static float surfaceArea(const BoundingBox& b)
	{
		glm::vec3 d = glm::max(b.max - b.min, glm::vec3(0.0f));
		return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
	}

	// pbrt EvaluateCost: power x solid-angle measure of the orientation cone x area,
	// regularised towards cube-like boxes along the split axis.
	static float cost(const Bounds& b, const BoundingBox& parent, int dim)
	{
		float to = safeAcos(b.cosO), te = safeAcos(b.cosE);
		float tw = std::min(to + te, pi);
		float so = std::sin(to);
		float mOmega = 2.0f * pi * (1.0f - b.cosO) +
			pi / 2.0f * (2.0f * tw * so - std::cos(to - 2.0f * tw) - 2.0f * to * so + b.cosO);
		glm::vec3 d = parent.max - parent.min;
		float kr = d[dim] > 0.0f ? std::max({ d.x, d.y, d.z }) / d[dim] : 1.0f;
		return b.phi * mOmega * kr * surfaceArea(b.box);
	}

	// Returns the node's bounds; appends nodes depth-first (left child = next node).
	Bounds build(std::vector<Bounded>& items, int start, int end, uint32_t trail, int depth)
	{
		maxDepthSeen = std::max(maxDepthSeen, depth);
		int nodeIndex = static_cast<int>(nodes.size());
		nodes.emplace_back();

		if (end - start == 1)
		{
			const Bounded& it = items[start];
			lights[it.light].bitTrail = static_cast<int>(trail);
			writeNode(nodeIndex, it.b, it.light, true);
			return it.b;
		}

		BoundingBox bounds, centroids;
		for (int i = start; i < end; ++i)
		{
			bounds.expand(items[i].b.box);
			glm::vec3 c = 0.5f * (items[i].b.box.min + items[i].b.box.max);
			centroids.expand(c);
		}

		// Bucketed SAH over all three axes; deep in the tree fall back to median splits so
		// bit trails stay within 32 levels.
		int mid = -1;
		if (depth < 14)
		{
			constexpr int kBuckets = 12;
			float bestCost = std::numeric_limits<float>::max();
			int bestDim = -1, bestBucket = -1;
			for (int dim = 0; dim < 3; ++dim)
			{
				float extent = centroids.max[dim] - centroids.min[dim];
				if (extent <= 0.0f)
					continue;
				Bounds buckets[kBuckets];
				for (int i = start; i < end; ++i)
				{
					float c = 0.5f * (items[i].b.box.min[dim] + items[i].b.box.max[dim]);
					int bi = std::min(kBuckets - 1, static_cast<int>(kBuckets * (c - centroids.min[dim]) / extent));
					buckets[bi] = unionBounds(buckets[bi], items[i].b);
				}
				for (int split = 0; split < kBuckets - 1; ++split)
				{
					Bounds l, r;
					for (int k = 0; k <= split; ++k) l = unionBounds(l, buckets[k]);
					for (int k = split + 1; k < kBuckets; ++k) r = unionBounds(r, buckets[k]);
					if (l.phi == 0.0f || r.phi == 0.0f)
						continue;
					float c = cost(l, bounds, dim) + cost(r, bounds, dim);
					if (c > 0.0f && c < bestCost)
					{
						bestCost = c;
						bestDim = dim;
						bestBucket = split;
					}
				}
			}
			if (bestDim >= 0)
			{
				float extent = centroids.max[bestDim] - centroids.min[bestDim];
				auto it = std::partition(items.begin() + start, items.begin() + end, [&](const Bounded& b)
					{
						float c = 0.5f * (b.b.box.min[bestDim] + b.b.box.max[bestDim]);
						int bi = std::min(kBuckets - 1, static_cast<int>(kBuckets * (c - centroids.min[bestDim]) / extent));
						return bi <= bestBucket;
					});
				mid = static_cast<int>(it - items.begin());
			}
		}
		if (mid <= start || mid >= end)
		{
			int dim = 0;
			glm::vec3 e = centroids.max - centroids.min;
			if (e.y > e.x) dim = 1;
			if (e.z > e[dim]) dim = 2;
			mid = (start + end) / 2;
			std::nth_element(items.begin() + start, items.begin() + mid, items.begin() + end,
				[dim](const Bounded& a, const Bounded& b)
				{
					return a.b.box.min[dim] + a.b.box.max[dim] < b.b.box.min[dim] + b.b.box.max[dim];
				});
		}

		uint32_t bit = depth < 32 ? (1u << depth) : 0u;
		Bounds left = build(items, start, mid, trail, depth + 1);
		int rightIndex = static_cast<int>(nodes.size());
		Bounds right = build(items, mid, end, trail | bit, depth + 1);
		Bounds all = unionBounds(left, right);
		writeNode(nodeIndex, all, rightIndex, false);
		return all;
	}

	void writeNode(int index, const Bounds& b, int childOrLight, bool leaf)
	{
		GPULightNode& n = nodes[index];
		n.boundsMinPhi = glm::vec4(b.box.min, b.phi);
		n.boundsMaxCosO = glm::vec4(b.box.max, b.cosO);
		n.axisCosE = glm::vec4(b.axis, b.cosE);
		n.childOrLight = childOrLight;
		n.flags = (leaf ? 1 : 0) | (b.twoSided ? 2 : 0);
	}
};
