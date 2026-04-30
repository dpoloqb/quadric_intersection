#include <gtest/gtest.h>

#include "Mesh.hpp"
#include "Polyline.hpp"
#include "Vec3.hpp"

using qi::geometry::Vec3;

TEST(MeshTest, EmptyMeshHasZeroTriangles) {
    qi::mesh::Mesh m;
    EXPECT_EQ(m.triangleCount(), 0u);
    EXPECT_EQ(m.vertexCount(), 0u);
    EXPECT_TRUE(qi::mesh::isValidMesh(m));
}

TEST(MeshTest, ClearRemovesEverything) {
    qi::mesh::Mesh m;
    m.vertices = {Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0)};
    m.triangles = {{0, 1, 2}};
    m.normals = {Vec3(0, 0, 1), Vec3(0, 0, 1), Vec3(0, 0, 1)};

    m.clear();

    EXPECT_EQ(m.triangleCount(), 0u);
    EXPECT_EQ(m.vertexCount(), 0u);
    EXPECT_TRUE(m.normals.empty());
}

TEST(MeshTest, IsValidMeshAcceptsConsistentMesh) {
    qi::mesh::Mesh m;
    m.vertices = {Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1)};
    m.triangles = {{0, 1, 2}, {0, 1, 3}, {1, 2, 3}};
    EXPECT_TRUE(qi::mesh::isValidMesh(m));
}

TEST(MeshTest, IsValidMeshRejectsOutOfRangeIndex) {
    qi::mesh::Mesh m;
    m.vertices = {Vec3(0, 0, 0), Vec3(1, 0, 0)};
    m.triangles = {{0, 1, 5}};  // index 5 out of range
    EXPECT_FALSE(qi::mesh::isValidMesh(m));
}

TEST(MeshTest, IsValidMeshRejectsNormalCountMismatch) {
    qi::mesh::Mesh m;
    m.vertices = {Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0)};
    m.triangles = {{0, 1, 2}};
    m.normals = {Vec3(0, 0, 1)};  // only 1 normal for 3 vertices
    EXPECT_FALSE(qi::mesh::isValidMesh(m));
}

TEST(PolylineTest, DefaultIsEmptyAndOpen) {
    qi::mesh::Polyline p;
    EXPECT_TRUE(p.points.empty());
    EXPECT_FALSE(p.closed);
}

TEST(SegmentTest, HoldsTwoEndpoints) {
    qi::mesh::Segment s{Vec3(0, 0, 0), Vec3(1, 1, 1)};
    EXPECT_DOUBLE_EQ(s.a.x(), 0.0);
    EXPECT_DOUBLE_EQ(s.b.norm(), std::sqrt(3.0));
}
