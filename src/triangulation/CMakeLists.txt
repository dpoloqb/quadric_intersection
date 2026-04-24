add_library(qi_triangulation STATIC
    version.cpp
    version.hpp
)
target_include_directories(qi_triangulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qi_triangulation
    PUBLIC
        Eigen3::Eigen
        qi_geometry
        qi_mesh
)
