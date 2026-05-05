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

// Discontinuity-aware triangulation: HyperboloidTwoSheet has two sheets
// (z >= c upper, z <= -c lower) joined nowhere. Without the fix, the
// parametric triangulator emits a "spike" of triangles bridging the two
// sheets through z=0. The fix declares vDiscontinuities() = {0.0} and skips
// quads that span the gap. Verify: no triangle has vertices on both sheets.
TEST(ParametricTriangulatorTest, HyperboloidTwoSheetNoBridgeBetweenSheets) {
    auto q = qi::geometry::createQuadric("hyperboloid_two_sheet", {1.0, 1.0, 1.0, 1.0});
    BoundingBox bbox(Vec3(-5, -5, -5), Vec3(5, 5, 5));
    Mesh m = ParametricTriangulator(ParametricParams{32, 32}).triangulate(*q, bbox);
    ASSERT_GT(m.triangleCount(), 0u);

    // Each sheet has |z| > c=1. After the fix, no triangle can have one
    // vertex on the upper sheet and another on the lower. Allow a bit of
    // slack near the threshold (clipping by bbox can move a vertex onto
    // a face).
    const double sheetThreshold = 0.5;  // safely below cosh(0)=1 for the bridge
    int crossingTriangles = 0;
    for (const auto& tri : m.triangles) {
        const double z0 = m.vertices[tri[0]].z();
        const double z1 = m.vertices[tri[1]].z();
        const double z2 = m.vertices[tri[2]].z();
        const bool anyUpper = z0 > sheetThreshold || z1 > sheetThreshold || z2 > sheetThreshold;
        const bool anyLower = z0 < -sheetThreshold || z1 < -sheetThreshold || z2 < -sheetThreshold;
        if (anyUpper && anyLower) ++crossingTriangles;
    }
    EXPECT_EQ(crossingTriangles, 0);
}

// After skipping the bridge quad, the lower sheet of HyperboloidTwoSheet
// loses its apex (parametric(u, 0) returns the *upper* apex (0, 0, +c)).
// The closingApexBelowV() API + triangulator fan must seal the lower sheet
// at (0, 0, -c). Verify some triangle vertex sits at z ≈ -c on-axis, both
// when the discontinuity falls on a grid line (vSteps even) and when it
// does not (vSteps odd → upper apex also synthesized).
TEST(ParametricTriangulatorTest, HyperboloidTwoSheetClosingApexFan) {
    auto q = qi::geometry::createQuadric("hyperboloid_two_sheet", {1.0, 1.0, 1.0, 1.0});
    BoundingBox bbox(Vec3(-5, -5, -5), Vec3(5, 5, 5));

    for (int vSteps : {32, 33}) {
        Mesh m = ParametricTriangulator(ParametricParams{32, vSteps}).triangulate(*q, bbox);
        bool hasLowerApex = false;
        bool hasUpperApex = false;
        for (const auto& v : m.vertices) {
            const double rxy = std::hypot(v.x(), v.y());
            if (rxy < 1e-6 && std::abs(v.z() + 1.0) < 1e-6) hasLowerApex = true;
            if (rxy < 1e-6 && std::abs(v.z() - 1.0) < 1e-6) hasUpperApex = true;
        }
        EXPECT_TRUE(hasLowerApex) << "vSteps=" << vSteps;
        EXPECT_TRUE(hasUpperApex) << "vSteps=" << vSteps;
    }
}

TEST(ParametricTriangulatorTest, HyperbolicCylinderNoBridgeBetweenBranches) {
    auto q = qi::geometry::createQuadric("hyperbolic_cylinder", {1.0, 1.0, 1.0, 1.0});
    BoundingBox bbox(Vec3(-5, -5, -5), Vec3(5, 5, 5));
    Mesh m = ParametricTriangulator(ParametricParams{32, 32}).triangulate(*q, bbox);
    ASSERT_GT(m.triangleCount(), 0u);

    // Right branch x >= a=1, left x <= -1.
    const double branchThreshold = 0.5;
    int crossingTriangles = 0;
    for (const auto& tri : m.triangles) {
        const double x0 = m.vertices[tri[0]].x();
        const double x1 = m.vertices[tri[1]].x();
        const double x2 = m.vertices[tri[2]].x();
        const bool anyRight = x0 > branchThreshold || x1 > branchThreshold || x2 > branchThreshold;
        const bool anyLeft = x0 < -branchThreshold || x1 < -branchThreshold || x2 < -branchThreshold;
        if (anyRight && anyLeft) ++crossingTriangles;
    }
    EXPECT_EQ(crossingTriangles, 0);
}
