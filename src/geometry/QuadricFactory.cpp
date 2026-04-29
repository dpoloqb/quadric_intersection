#include "QuadricFactory.hpp"

#include <stdexcept>

#include "Cone.hpp"
#include "Ellipsoid.hpp"
#include "EllipticCylinder.hpp"
#include "EllipticParaboloid.hpp"
#include "HyperbolicCylinder.hpp"
#include "HyperbolicParaboloid.hpp"
#include "HyperboloidOneSheet.hpp"
#include "HyperboloidTwoSheet.hpp"
#include "ParabolicCylinder.hpp"

namespace qi::geometry {

std::unique_ptr<Quadric> createQuadric(const std::string& type, const QuadricParams& params) {
    if (type == "ellipsoid") {
        return std::make_unique<Ellipsoid>(params.a, params.b, params.c);
    }
    if (type == "hyperboloid_one_sheet") {
        return std::make_unique<HyperboloidOneSheet>(params.a, params.b, params.c);
    }
    if (type == "hyperboloid_two_sheet") {
        return std::make_unique<HyperboloidTwoSheet>(params.a, params.b, params.c);
    }
    if (type == "elliptic_paraboloid") {
        return std::make_unique<EllipticParaboloid>(params.a, params.b);
    }
    if (type == "hyperbolic_paraboloid") {
        return std::make_unique<HyperbolicParaboloid>(params.a, params.b);
    }
    if (type == "cone") {
        return std::make_unique<Cone>(params.a, params.b, params.c);
    }
    if (type == "elliptic_cylinder") {
        return std::make_unique<EllipticCylinder>(params.a, params.b);
    }
    if (type == "hyperbolic_cylinder") {
        return std::make_unique<HyperbolicCylinder>(params.a, params.b);
    }
    if (type == "parabolic_cylinder") {
        return std::make_unique<ParabolicCylinder>(params.p);
    }
    throw std::invalid_argument("createQuadric: unknown type '" + type + "'");
}

std::vector<std::string> knownQuadricTypes() {
    return {
        "ellipsoid",
        "hyperboloid_one_sheet",
        "hyperboloid_two_sheet",
        "elliptic_paraboloid",
        "hyperbolic_paraboloid",
        "cone",
        "elliptic_cylinder",
        "hyperbolic_cylinder",
        "parabolic_cylinder",
    };
}

}  // namespace qi::geometry
