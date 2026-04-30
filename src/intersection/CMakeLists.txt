add_library(qi_intersection STATIC
    Orient3d.cpp
    Orient3d.hpp
    TriangleTriangle.cpp
    TriangleTriangle.hpp
)
target_include_directories(qi_intersection PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qi_intersection
    PUBLIC
        Eigen3::Eigen
        qi_geometry
        qi_mesh
)
