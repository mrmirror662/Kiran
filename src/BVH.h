#pragma once
#include "primitives.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <limits>
#include <numeric>
struct BoundingBox {
  alignas(16) glm::vec3 min;
  alignas(16) glm::vec3 max;

  BoundingBox() {
    min = glm::vec3(std::numeric_limits<float>::max());
    max = glm::vec3(std::numeric_limits<float>::lowest());
  }

  void expand(const glm::vec3 &point) {
    min = glm::min(min, point);
    max = glm::max(max, point);
  }

  void expand(const BoundingBox &box) {
    expand(box.min);
    expand(box.max);
  }

  bool intersects(const BoundingBox &other) const {
    return (min.x <= other.max.x && max.x >= other.min.x) &&
           (min.y <= other.max.y && max.y >= other.min.y) &&
           (min.z <= other.max.z && max.z >= other.min.z);
  }

  bool contains(const glm::vec3 &point) const {
    return (point.x >= min.x && point.x <= max.x) &&
           (point.y >= min.y && point.y <= max.y) &&
           (point.z >= min.z && point.z <= max.z);
  }
};

BoundingBox getAABB(const Triangle &t) {
  auto v0 = t.v0;
  auto v1 = t.v1;
  auto v2 = t.v2;

  auto min_x = std::min({v0.x, v1.x, v2.x});
  auto min_y = std::min({v0.y, v1.y, v2.y});
  auto min_z = std::min({v0.z, v1.z, v2.z});
  auto max_x = std::max({v0.x, v1.x, v2.x});
  auto max_y = std::max({v0.y, v1.y, v2.y});
  auto max_z = std::max({v0.z, v1.z, v2.z});

  // Adding a small thickness to each axis to ensure the bounding box is not
  // degenerate
  float thickness = 1e-3f;

  BoundingBox bb;
  bb.min = glm::vec3(min_x - thickness, min_y - thickness, min_z - thickness);
  bb.max = glm::vec3(max_x, max_y, max_z);

  return bb;
}

struct BVHNode {
  BoundingBox box;
  int left;  // Index of the left child
  int right; // Index of the right child
  int start; // Start index of triangles in this node
  int end;   // End index of triangles in this node

  BVHNode() : left(-1), right(-1), start(-1), end(-1) {}
};

class BVH {
public:
  BVH(const std::vector<Triangle> &triangles, int maxLeafSize)
      : triangles(triangles), maxLeafSize(maxLeafSize) {
    buildBVH();
  }

  const std::vector<BVHNode> &getNodes() const { return nodes; }

  std::vector<Triangle> triangles;
  std::vector<BVHNode> nodes;
  std::vector<int> triangleIndices;
  int maxLeafSize;
  void buildBVH() {
    nodes.clear();
    nodes.reserve(triangles.size() * 2);
    triangleIndices.resize(triangles.size());
    std::iota(triangleIndices.begin(), triangleIndices.end(), 0);
    buildNode(-1, 0, triangles.size());
  }

  int buildNode(int parent, int start, int end) {
    int nodeIndex = nodes.size();
    nodes.emplace_back();
    BVHNode &node = nodes.back();
    node.start = start;
    node.end = end;

    BoundingBox box;
    for (int i = start; i < end; ++i) {
      int triangleIndex = triangleIndices[i];
      const Triangle &tri = triangles[triangleIndex];
      box.expand(getAABB(tri));
    }
    node.box = box;

    int numTriangles = end - start;
    if (numTriangles <= 2) {
      node.left = -1;
      node.right = -1;
      return nodeIndex;
    }

    glm::vec3 centroidMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 centroidMax = glm::vec3(std::numeric_limits<float>::lowest());
    for (int i = start; i < end; ++i) {
      int triangleIndex = triangleIndices[i];
      const Triangle &tri = triangles[triangleIndex];
      glm::vec3 centroid = (tri.v0 + tri.v1 + tri.v2) / 3.0f;
      centroidMin = glm::min(centroidMin, centroid);
      centroidMax = glm::max(centroidMax, centroid);
    }

    glm::vec3 extent = centroidMax - centroidMin;
    int axis = extent.x > extent.y ? (extent.x > extent.z ? 0 : 2)
                                   : (extent.y > extent.z ? 1 : 2);

    int mid = (start + end) / 2;
    std::nth_element(
        triangleIndices.begin() + start, triangleIndices.begin() + mid,
        triangleIndices.begin() + end, [this, axis](int lhs, int rhs) {
          const Triangle &a = triangles[lhs];
          const Triangle &b = triangles[rhs];
          float centroidA = (a.v0[axis] + a.v1[axis] + a.v2[axis]) / 3.0f;
          float centroidB = (b.v0[axis] + b.v1[axis] + b.v2[axis]) / 3.0f;
          return centroidA < centroidB;
        });

    node.left = buildNode(nodeIndex, start, mid);
    node.right = buildNode(nodeIndex, mid, end);
    return nodeIndex;
  }
};
