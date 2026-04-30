#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

#include "BoundingBox.hpp"
#include "Ellipsoid.hpp"
#include "MarchingCubes.hpp"
#include "MarchingCubesParams.hpp"
#include "Mesh.hpp"
#include "ParametricParams.hpp"
#include "ParametricTriangulator.hpp"
#include "QuadricFactory.hpp"
#include "Vec3.hpp"
#include "test_utils.hpp"

using qi::geometry::BoundingBox;
using qi::geometry::Ellipsoid;
using qi::geometry::Quadric;
using qi::geometry::QuadricParams;
using qi::geometry::Vec3;
using qi::mesh::Mesh;
using qi::triangulation::MarchingCubes;
using qi::triangulation::MarchingCubesParams;
using qi::triangulation::ParametricParams;
using qi::triangulation::ParametricTriangulator;

namespace {

double maxAbsImplicit(const Mesh& mesh, const Quadric& q) {
    double m = 0.0;
    for (const auto& v : mesh.vertices) {
        m = std::max(m, std::abs(q.implicit(v)));
    }
    return m;
}

}  // namespace

TEST(MarchingCubesTest, MethodNameIsStable) {
    MarchingCubes mc;
    EXPECT_EQ(mc.methodName(), "marching_cubes");
}

TEST(MarchingCubesTest, UnitSphereHasTriangles) {
    Ellipsoid sphere(1.0, 1.0, 1.0);
    BoundingBox bbox(Vec3(-2, -2, -2), Vec3(2, 2, 2));
    MarchingCubes mc(MarchingCubesParams{32});

    Mesh mesh = mc.triangulate(sphere, bbox);

    EXPECT_GT(mesh.triangleCount(), 0u);
    EXPECT_TRUE(qi::mesh::isValidMesh(mesh));
    qi::test::expectMeshInsideBbox(mesh, bbox, qi::test::kEpsTight);
    // MC vertex tolerance scales as diagonal/resolution.
    const double tol = bbox.diagonal() / 32.0;
    qi::test::expectMeshLiesOnSurface(mesh, sphere, tol);
}

TEST(MarchingCubesTest, HigherResolutionGivesBetterAccuracy) {
    Ellipsoid sphere(1.0, 1.0, 1.0);
    BoundingBox bbox(Vec3(-2, -2, -2), Vec3(2, 2, 2));

    MarchingCubes coarse(MarchingCubesParams{16});
    MarchingCubes fine(MarchingCubesParams{64});

    Mesh m1 = coarse.triangulate(sphere, bbox);
    Mesh m2 = fine.triangulate(sphere, bbox);

    EXPECT_GT(m1.triangleCount(), 0u);
    EXPECT_GT(m2.triangleCount(), 0u);
    EXPECT_GT(m2.triangleCount(), m1.triangleCount());

    const double err1 = maxAbsImplicit(m1, sphere);
    const double err2 = maxAbsImplicit(m2, sphere);
    EXPECT_LT(err2, err1);
}

TEST(MarchingCubesTest, EmptyMeshWhenSurfaceMissesBbox) {
    // Sphere of radius 1 centered at origin is entirely outside the bbox far away.
    Ellipsoid sphere(1.0, 1.0, 1.0);
    BoundingBox far(Vec3(10, 10, 10), Vec3(11, 11, 11));
    MarchingCubes mc(MarchingCubesParams{16});

    Mesh mesh = mc.triangulate(sphere, far);
    EXPECT_EQ(mesh.triangleCount(), 0u);
}

TEST(MarchingCubesTest, AllNineTypesProduceNonEmptyMesh) {
    BoundingBox bbox(Vec3(-3, -3, -3), Vec3(3, 3, 3));
    MarchingCubes mc(MarchingCubesParams{24});
    QuadricParams params{1.0, 1.0, 1.0, 1.0};

    for (const auto& type : qi::geometry::knownQuadricTypes()) {
        auto q = qi::geometry::createQuadric(type, params);
        ASSERT_NE(q, nullptr);
        Mesh mesh = mc.triangulate(*q, bbox);
        EXPECT_GT(mesh.triangleCount(), 0u) << "empty mesh for type=" << type;
        EXPECT_TRUE(qi::mesh::isValidMesh(mesh)) << "invalid mesh for type=" << type;
        qi::test::expectMeshInsideBbox(mesh, bbox, qi::test::kEpsTight);
    }
}

TEST(MarchingCubesTest, VerticesLieOnSurfaceForAllTypes) {
    BoundingBox bbox(Vec3(-3, -3, -3), Vec3(3, 3, 3));
    const int resolution = 32;
    MarchingCubes mc(MarchingCubesParams{resolution});
    QuadricParams params{1.0, 1.0, 1.0, 1.0};

    const double tol = bbox.diagonal() / resolution;
    for (const auto& type : qi::geometry::knownQuadricTypes()) {
        auto q = qi::geometry::createQuadric(type, params);
        Mesh mesh = mc.triangulate(*q, bbox);
        // Each vertex should satisfy |implicit| < diagonal/resolution.
        for (const auto& v : mesh.vertices) {
            EXPECT_LT(std::abs(q->implicit(v)), tol)
                << "type=" << type << " v=" << v.transpose();
        }
    }
}

TEST(MarchingCubesTest, ResolutionOneStillProducesValidMesh) {
    Ellipsoid sphere(1.0, 1.0, 1.0);
    BoundingBox bbox(Vec3(-2, -2, -2), Vec3(2, 2, 2));
    MarchingCubes mc(MarchingCubesParams{1});

    Mesh mesh = mc.triangulate(sphere, bbox);
    EXPECT_TRUE(qi::mesh::isValidMesh(mesh));
    // With resolution=1 the cube fully encloses the sphere; depending on corner
    // signs, the algorithm may produce 0 triangles. Either way, no crash.
    SUCCEED();
}

// ---- ParametricTriangulator ----

TEST(ParametricTriangulatorTest, MethodNameIsStable) {
    ParametricTriangulator pt;
    EXPECT_EQ(pt.methodName(), "parametric");
}

TEST(ParametricTriangulatorTest, EllipsoidExactOnSurface) {
    Ellipsoid e(2.0, 3.0, 4.0);
    // Bbox big enough that no clipping happens — vertices stay parametric.
    BoundingBox bbox(Vec3(-10, -10, -10), Vec3(10, 10, 10));
    ParametricTriangulator pt(ParametricParams{40, 40});

    Mesh mesh = pt.triangulate(e, bbox);
    EXPECT_GT(mesh.triangleCount(), 0u);
    EXPECT_TRUE(qi::mesh::isValidMesh(mesh));
    qi::test::expectMeshLiesOnSurface(mesh, e, qi::test::kEpsTight);
    qi::test::expectMeshInsideBbox(mesh, bbox, qi::test::kEpsTight);
}

TEST(ParametricTriangulatorTest, AllNineTypesProduceNonEmptyMesh) {
    // Bbox big enough to contain every default-parameter surface fully:
    // EllipticParaboloid reaches z=v²=25 at v=5; HyperbolicParaboloid reaches
    // |z|=u²+v²=50; ParabolicCylinder reaches x=u²/2=12.5. Using [-30,30]³ to
    // ensure no clipping happens, so vertices stay parametric (exact).
    BoundingBox bbox(Vec3(-30, -30, -30), Vec3(30, 30, 30));
    ParametricTriangulator pt(ParametricParams{30, 30});
    QuadricParams params{1.0, 1.0, 1.0, 1.0};

    for (const auto& type : qi::geometry::knownQuadricTypes()) {
        auto q = qi::geometry::createQuadric(type, params);
        ASSERT_NE(q, nullptr);
        Mesh mesh = pt.triangulate(*q, bbox);
        EXPECT_GT(mesh.triangleCount(), 0u) << "empty mesh for type=" << type;
        EXPECT_TRUE(qi::mesh::isValidMesh(mesh)) << "invalid mesh for type=" << type;
        qi::test::expectMeshInsideBbox(mesh, bbox, qi::test::kEpsTight);
        qi::test::expectMeshLiesOnSurface(mesh, *q, qi::test::kEpsTight);
    }
}

TEST(ParametricTriangulatorTest, ClippedByBboxKeepsVerticesInside) {
    // Cylinder is "infinite" in z (vRange=[-10,10]); bbox is small.
    auto cyl = qi::geometry::createQuadric("elliptic_cylinder",
                                            qi::geometry::QuadricParams{1.0, 1.0, 1.0, 1.0});
    BoundingBox bbox(Vec3(-2, -2, -1), Vec3(2, 2, 1));
    ParametricTriangulator pt(ParametricParams{40, 60});

    Mesh mesh = pt.triangulate(*cyl, bbox);
    EXPECT_GT(mesh.triangleCount(), 0u);
    EXPECT_TRUE(qi::mesh::isValidMesh(mesh));
    qi::test::expectMeshInsideBbox(mesh, bbox, qi::test::kEpsLoose);
}

TEST(ParametricTriangulatorTest, HigherStepsGiveMoreTriangles) {
    Ellipsoid e(1.0, 1.0, 1.0);
    BoundingBox bbox(Vec3(-10, -10, -10), Vec3(10, 10, 10));

    Mesh coarse = ParametricTriangulator(ParametricParams{20, 20}).triangulate(e, bbox);
    Mesh fine   = ParametricTriangulator(ParametricParams{60, 60}).triangulate(e, bbox);

    EXPECT_GT(fine.triangleCount(), coarse.triangleCount());
}
