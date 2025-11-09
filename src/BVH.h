#pragma once
#include "primitives.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <limits>

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

class BVH
{
public:
	BVH(std::vector<Triangle> triangles, int maxDepth)
		: triangles(std::move(triangles)), maxDepth(maxDepth)
	{
		buildBVH();
	}

	const std::vector<BVHNode>& getNodes() const { return nodes; }
	const std::vector<Triangle>& getTriangles() const { return triangles; }


	std::vector<Triangle> triangles;
	std::vector<BVHNode> nodes;
	int maxDepth;

	void buildBVH()
	{
		nodes.clear();
		nodes.reserve(triangles.size() * 2);
		buildNode(-1, 0, triangles.size(), 0);
	}

	int buildNode(int parent, int start, int end, int depth)
	{
		int nodeIndex = nodes.size();
		nodes.emplace_back();
		BVHNode& node = nodes.back();
		node.start = start;
		node.end = end;

		// Build bounding box for current range
		BoundingBox box;
		for (int i = start; i < end; ++i)
		{
			const Triangle& tri = triangles[i];
			box.expand(BoundingBox::getAABB(tri));
		}

		node.box = box;

		int numTriangles = end - start;

		// Compute centroid bounds
		glm::vec3 centroidMin = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 centroidMax = glm::vec3(std::numeric_limits<float>::lowest());
		for (int i = start; i < end; ++i)
		{
			const Triangle& tri = triangles[i];
			glm::vec3 centroid = (tri.v0 + tri.v1 + tri.v2) / 3.0f;
			centroidMin = glm::min(centroidMin, centroid);
			centroidMax = glm::max(centroidMax, centroid);
		}

		glm::vec3 extent = centroidMax - centroidMin;
		int axis = extent.x > extent.y ? (extent.x > extent.z ? 0 : 2)
			: (extent.y > extent.z ? 1 : 2);

		// Fallback to leaf if extent is too small or other termination criteria
		if (numTriangles <= 4 || depth >= maxDepth || extent[axis] < 1e-5f)
		{
			node.left = -1;
			node.right = -1;
			return nodeIndex;
		}

		// Partition triangles directly using nth_element by centroid along split axis
		int mid = (start + end) / 2;
		std::nth_element(
			triangles.begin() + start,
			triangles.begin() + mid,
			triangles.begin() + end,
			[axis](const Triangle& a, const Triangle& b)
			{
				float centroidA = (a.v0[axis] + a.v1[axis] + a.v2[axis]) / 3.0f;
				float centroidB = (b.v0[axis] + b.v1[axis] + b.v2[axis]) / 3.0f;
				return centroidA < centroidB;
			});

		node.left = buildNode(nodeIndex, start, mid, depth + 1);
		node.right = buildNode(nodeIndex, mid, end, depth + 1);
		return nodeIndex;
	}
};