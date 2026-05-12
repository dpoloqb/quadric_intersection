#include "BvhIntersector.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>

#include "BoundingBox.hpp"
#include "Mesh.hpp"
#include "TriangleTriangle.hpp"
#include "Vec3.hpp"

namespace qi::intersection {

using qi::geometry::BoundingBox;
using qi::geometry::Vec3;
using qi::mesh::Mesh;
using qi::mesh::Segment;

namespace {

constexpr std::size_t kLeafSize = 8;

struct BvhNode {
    BoundingBox bbox;
    std::int32_t left = -1;
    std::int32_t right = -1;
    std::int32_t triBegin = 0;
    std::int32_t triEnd = 0;
    bool isLeaf() const { return left < 0 && right < 0; }
};

bool aabbOverlap(const BoundingBox& a, const BoundingBox& b) {
    return (a.min().array() <= b.max().array()).all() &&
           (b.min().array() <= a.max().array()).all();
}

BoundingBox triangleBbox(const Mesh& mesh, std::size_t triIdx) {
    const auto& t = mesh.triangles[triIdx];
    const Vec3& v0 = mesh.vertices[t[0]];
    const Vec3& v1 = mesh.vertices[t[1]];
    const Vec3& v2 = mesh.vertices[t[2]];
    return BoundingBox(v0.cwiseMin(v1).cwiseMin(v2),
                       v0.cwiseMax(v1).cwiseMax(v2));
}

class Bvh {
public:
    void build(const Mesh& mesh) {
        const std::size_t n = mesh.triangleCount();
        triIndices_.resize(n);
        std::iota(triIndices_.begin(), triIndices_.end(), 0u);

        triBboxes_.clear();
        triBboxes_.reserve(n);
        triCentroids_.clear();
        triCentroids_.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            BoundingBox b = triangleBbox(mesh, i);
            triBboxes_.push_back(b);
            triCentroids_.push_back(b.center());
        }

        nodes_.clear();
        if (n == 0) return;
        nodes_.reserve(2 * n);
        nodes_.push_back({});
        buildRecursive(0, 0, n);
    }

    void query(const BoundingBox& q, std::vector<std::size_t>& outTriIdx) const {
        if (nodes_.empty()) return;
        queryRecursive(0, q, outTriIdx);
    }

    // Walk the tree in pre-order, collecting one entry per node with its
    // world-space AABB and depth. Used by visualization code only.
    std::vector<BvhVizNode> enumerateNodes() const {
        std::vector<BvhVizNode> out;
        if (nodes_.empty()) return out;
        out.reserve(nodes_.size());
        enumerateRecursive(0, 0, out);
        return out;
    }

private:
    void buildRecursive(std::int32_t nodeIdx, std::size_t triBegin, std::size_t triEnd) {
        BoundingBox bbox = triBboxes_[triIndices_[triBegin]];
        for (std::size_t i = triBegin + 1; i < triEnd; ++i) {
            const auto& tb = triBboxes_[triIndices_[i]];
            bbox = BoundingBox(bbox.min().cwiseMin(tb.min()),
                               bbox.max().cwiseMax(tb.max()));
        }
        nodes_[nodeIdx].bbox = bbox;

        const std::size_t count = triEnd - triBegin;
        if (count <= kLeafSize) {
            nodes_[nodeIdx].triBegin = static_cast<std::int32_t>(triBegin);
            nodes_[nodeIdx].triEnd = static_cast<std::int32_t>(triEnd);
            return;
        }

        const Vec3 size = bbox.max() - bbox.min();
        int axis = 0;
        if (size.y() > size[axis]) axis = 1;
        if (size.z() > size[axis]) axis = 2;

        const std::size_t mid = triBegin + count / 2;
        std::nth_element(triIndices_.begin() + triBegin,
                         triIndices_.begin() + mid,
                         triIndices_.begin() + triEnd,
                         [&](std::size_t a, std::size_t b) {
                             return triCentroids_[a][axis] < triCentroids_[b][axis];
                         });

        const std::int32_t leftIdx = static_cast<std::int32_t>(nodes_.size());
        nodes_.push_back({});
        const std::int32_t rightIdx = static_cast<std::int32_t>(nodes_.size());
        nodes_.push_back({});

        nodes_[nodeIdx].left = leftIdx;
        nodes_[nodeIdx].right = rightIdx;

        buildRecursive(leftIdx, triBegin, mid);
        buildRecursive(rightIdx, mid, triEnd);
    }

    void enumerateRecursive(std::int32_t nodeIdx, int depth,
                            std::vector<BvhVizNode>& out) const {
        const BvhNode& node = nodes_[nodeIdx];
        out.push_back({node.bbox, depth, node.isLeaf()});
        if (!node.isLeaf()) {
            enumerateRecursive(node.left, depth + 1, out);
            enumerateRecursive(node.right, depth + 1, out);
        }
    }

    void queryRecursive(std::int32_t nodeIdx,
                        const BoundingBox& q,
                        std::vector<std::size_t>& out) const {
        const BvhNode& node = nodes_[nodeIdx];
        if (!aabbOverlap(node.bbox, q)) return;
        if (node.isLeaf()) {
            for (std::int32_t i = node.triBegin; i < node.triEnd; ++i) {
                if (aabbOverlap(triBboxes_[triIndices_[i]], q)) {
                    out.push_back(triIndices_[i]);
                }
            }
            return;
        }
        queryRecursive(node.left, q, out);
        queryRecursive(node.right, q, out);
    }

    std::vector<std::size_t> triIndices_;
    std::vector<BoundingBox> triBboxes_;
    std::vector<Vec3> triCentroids_;
    std::vector<BvhNode> nodes_;
};

}  // namespace

std::vector<BvhVizNode> buildBvhForVisualization(const Mesh& mesh) {
    Bvh bvh;
    bvh.build(mesh);
    return bvh.enumerateNodes();
}

std::vector<Segment> BvhIntersector::findSegments(const Mesh& a, const Mesh& b) {
    Bvh bvh;
    bvh.build(a);

    std::vector<Segment> result;
    std::vector<std::size_t> hits;
    hits.reserve(32);

    for (const auto& tB : b.triangles) {
        const Vec3& a2 = b.vertices[tB[0]];
        const Vec3& b2 = b.vertices[tB[1]];
        const Vec3& c2 = b.vertices[tB[2]];
        const BoundingBox tBox(a2.cwiseMin(b2).cwiseMin(c2),
                               a2.cwiseMax(b2).cwiseMax(c2));

        hits.clear();
        bvh.query(tBox, hits);

        for (std::size_t idx : hits) {
            const auto& tA = a.triangles[idx];
            const Vec3& a1 = a.vertices[tA[0]];
            const Vec3& b1 = a.vertices[tA[1]];
            const Vec3& c1 = a.vertices[tA[2]];
            if (auto seg = intersectTriangles(a1, b1, c1, a2, b2, c2)) {
                result.push_back(*seg);
            }
        }
    }
    return result;
}

}  // namespace qi::intersection
