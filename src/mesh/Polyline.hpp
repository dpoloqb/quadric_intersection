#pragma once

#include <vector>

#include "Vec3.hpp"

namespace qi::mesh {

struct Segment {
    qi::geometry::Vec3 a;
    qi::geometry::Vec3 b;
};

struct Polyline {
    std::vector<qi::geometry::Vec3> points;
    bool closed = false;
};

}  // namespace qi::mesh
