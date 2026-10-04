#pragma once
#include "primitives.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <cmath>
#include <limits>
#include <vector>

struct BoundingBox
{
	alignas(16) glm::vec3 min;
	alignas(16) glm::vec3 max;

	BoundingBox()
	{
		min = glm::vec3(std::numeric_limits<float>::max());
		max = glm::vec3(std::numeric_limits<float>::lowest());
	}

	void expand(const glm::vec3& point)
	{
		min = glm::min(min, point);
		max = glm::max(max, point);
	}

	void expand(const BoundingBox& box)
	{
		expand(box.min);
		expand(box.max);
	}

	bool intersects(const BoundingBox& other) const
	{
		return (min.x <= other.max.x && max.x >= other.min.x) &&
			(min.y <= other.max.y && max.y >= other.min.y) &&
			(min.z <= other.max.z && max.z >= other.min.z);
	}

	bool contains(const glm::vec3& point) const
	{
		return (point.x >= min.x && point.x <= max.x) &&
			(point.y >= min.y && point.y <= max.y) &&
			(point.z >= min.z && point.z <= max.z);
	}

	static BoundingBox getAABB(const Triangle& t)
	{
		auto v0 = t.v0;
		auto v1 = t.v1;
		auto v2 = t.v2;

		auto min_x = std::min({ v0.x, v1.x, v2.x });
		auto min_y = std::min({ v0.y, v1.y, v2.y });
		auto min_z = std::min({ v0.z, v1.z, v2.z });
		auto max_x = std::max({ v0.x, v1.x, v2.x });
		auto max_y = std::max({ v0.y, v1.y, v2.y });
		auto max_z = std::max({ v0.z, v1.z, v2.z });

		// Expand bounds by 1 ULP to avoid floating-point edge misses
		BoundingBox bb;
		bb.min = glm::vec3(
			std::nextafterf(min_x, -INFINITY),
			std::nextafterf(min_y, -INFINITY),
			std::nextafterf(min_z, -INFINITY));
		bb.max = glm::vec3(
			std::nextafterf(max_x, INFINITY),
			std::nextafterf(max_y, INFINITY),
			std::nextafterf(max_z, INFINITY));
		return bb;
	}
};

struct BVHNode
{
	BoundingBox box;
	int left;
	int right;
	int start;
	int end;

	BVHNode() : left(-1), right(-1), start(-1), end(-1) {}
};

// Binned-SAH BVH builder shared by both acceleration levels (BLAS over triangles,
// TLAS over instances). Reorders `items` so every node covers items[start, end).
template <typename T, typename BoundsFn, typename CentroidFn>
class BVHBuilder
{
public:
	BVHBuilder(std::vector<T>& items, std::vector<BVHNode>& nodes, int maxDepth, int leafSize,
		BoundsFn bounds, CentroidFn centroid)
		: items(items), nodes(nodes), maxDepth(maxDepth), leafSize(leafSize), bounds(bounds), centroid(centroid)
	{
		nodes.clear();
		nodes.reserve(items.size() * 2);
		buildNode(0, static_cast<int>(items.size()), 0);
	}

private:
	std::vector<T>& items;
	std::vector<BVHNode>& nodes;
	int maxDepth;
	int leafSize;
	static constexpr int kMaxLeafPrims = 63;
	BoundsFn bounds;
	CentroidFn centroid;

	static float surfaceArea(const BoundingBox& b)
	{
		glm::vec3 d = glm::max(b.max - b.min, glm::vec3(0.0f));
		return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
	}

	int buildNode(int start, int end, int depth)
	{
		int nodeIndex = static_cast<int>(nodes.size());
		nodes.emplace_back();
		nodes[nodeIndex].start = start;
		nodes[nodeIndex].end = end;

		BoundingBox box;
		glm::vec3 centroidMin(std::numeric_limits<float>::max());
		glm::vec3 centroidMax(std::numeric_limits<float>::lowest());
		for (int i = start; i < end; ++i)
		{
			box.expand(bounds(items[i]));
			glm::vec3 c = centroid(items[i]);
			centroidMin = glm::min(centroidMin, c);
			centroidMax = glm::max(centroidMax, c);
		}
		nodes[nodeIndex].box = box;

		glm::vec3 extent = centroidMax - centroidMin;
		int axis = extent.x > extent.y ? (extent.x > extent.z ? 0 : 2)
			: (extent.y > extent.z ? 1 : 2);
		int count = end - start;
		if (count <= 1)
			return nodeIndex;
		if (depth >= maxDepth || extent[axis] < 1e-5f)
		{
			// Forced leaf (depth cap, or centroids that cannot be separated). Leaves hold at
			// most kMaxLeafPrims (the wide traversal packs counts into 6 bits): split larger
			// ones at the median item, which always works, even past the depth cap.
			if (count <= kMaxLeafPrims)
				return nodeIndex;
			int mid = (start + end) / 2;
			std::nth_element(items.begin() + start, items.begin() + mid, items.begin() + end,
				[&](const T& a, const T& b) { return centroid(a)[axis] < centroid(b)[axis]; });
			int left = buildNode(start, mid, depth + 1);
			int right = buildNode(mid, end, depth + 1);
			nodes[nodeIndex].left = left;
			nodes[nodeIndex].right = right;
			return nodeIndex;
		}

		// Binned SAH along the widest centroid axis: minimise
		// area(L) * count(L) + area(R) * count(R) over bin boundaries.
		constexpr int kBins = 16;
		BoundingBox binBox[kBins];
		int binCount[kBins] = {};
		float scale = kBins / extent[axis];
		auto binOf = [&](const T& item)
			{
				int b = static_cast<int>((centroid(item)[axis] - centroidMin[axis]) * scale);
				return std::min(b, kBins - 1);
			};
		for (int i = start; i < end; ++i)
		{
			int b = binOf(items[i]);
			binCount[b]++;
			binBox[b].expand(bounds(items[i]));
		}

		float rightCost[kBins] = {};
		BoundingBox acc;
		int accCount = 0;
		for (int b = kBins - 1; b > 0; --b)
		{
			accCount += binCount[b];
			if (binCount[b]) acc.expand(binBox[b]);
			rightCost[b] = accCount ? surfaceArea(acc) * accCount : 0.0f;
		}
		float bestCost = std::numeric_limits<float>::max();
		int bestSplit = -1; // items in bins [0, bestSplit] go left
		acc = BoundingBox();
		accCount = 0;
		for (int b = 0; b < kBins - 1; ++b)
		{
			accCount += binCount[b];
			if (binCount[b]) acc.expand(binBox[b]);
			if (accCount == 0 || accCount == count)
				continue;
			float cost = surfaceArea(acc) * accCount + rightCost[b + 1];
			if (cost < bestCost)
			{
				bestCost = cost;
				bestSplit = b;
			}
		}

		// Leaf if small enough and splitting would not beat intersecting everything here
		// (a traversal step costs about half an intersection: measured best on Bistro).
		float leafCost = surfaceArea(box) * count;
		if (bestSplit < 0 || (count <= leafSize && 0.5f * surfaceArea(box) + bestCost >= leafCost))
			return nodeIndex;

		int mid = static_cast<int>(std::partition(items.begin() + start, items.begin() + end,
			[&](const T& item) { return binOf(item) <= bestSplit; }) - items.begin());
		if (mid == start || mid == end)
		{
			mid = (start + end) / 2;
			std::nth_element(items.begin() + start, items.begin() + mid, items.begin() + end,
				[&](const T& a, const T& b) { return centroid(a)[axis] < centroid(b)[axis]; });
		}

		// nodes may reallocate during recursion; assign through the index.
		int left = buildNode(start, mid, depth + 1);
		int right = buildNode(mid, end, depth + 1);
		nodes[nodeIndex].left = left;
		nodes[nodeIndex].right = right;
		return nodeIndex;
	}
};

template <typename T, typename BoundsFn, typename CentroidFn>
void buildBVH(std::vector<T>& items, std::vector<BVHNode>& nodes, int maxDepth, int leafSize,
	BoundsFn bounds, CentroidFn centroid)
{
	BVHBuilder<T, BoundsFn, CentroidFn>(items, nodes, maxDepth, leafSize, bounds, centroid);
}

// Triangle BVH (one BLAS).
class BVH
{
public:
	BVH(std::vector<Triangle> triangles, int maxDepth)
		: triangles(std::move(triangles)), maxDepth(maxDepth)
	{
		buildBVH(this->triangles, nodes, maxDepth, 8,
			[](const Triangle& t) { return BoundingBox::getAABB(t); },
			[](const Triangle& t) { return (t.v0 + t.v1 + t.v2) / 3.0f; });
	}

	const std::vector<BVHNode>& getNodes() const { return nodes; }
	const std::vector<Triangle>& getTriangles() const { return triangles; }

	std::vector<Triangle> triangles;
	std::vector<BVHNode> nodes;
	int maxDepth;
};
