add_library(qi_geometry STATIC
    Vec3.hpp
    Transform.cpp
    Transform.hpp
    BoundingBox.cpp
    BoundingBox.hpp
    Quadric.hpp
    Ellipsoid.cpp
    Ellipsoid.hpp
    HyperboloidOneSheet.cpp
    HyperboloidOneSheet.hpp
    HyperboloidTwoSheet.cpp
    HyperboloidTwoSheet.hpp
    EllipticParaboloid.cpp
    EllipticParaboloid.hpp
    HyperbolicParaboloid.cpp
    HyperbolicParaboloid.hpp
    Cone.cpp
    Cone.hpp
    EllipticCylinder.cpp
    EllipticCylinder.hpp
    HyperbolicCylinder.cpp
    HyperbolicCylinder.hpp
    ParabolicCylinder.cpp
    ParabolicCylinder.hpp
)
target_include_directories(qi_geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qi_geometry PUBLIC Eigen3::Eigen)
