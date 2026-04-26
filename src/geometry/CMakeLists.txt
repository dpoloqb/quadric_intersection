add_library(qi_geometry STATIC
    Vec3.hpp
    Transform.cpp
    Transform.hpp
    BoundingBox.cpp
    BoundingBox.hpp
)
target_include_directories(qi_geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qi_geometry PUBLIC Eigen3::Eigen)
