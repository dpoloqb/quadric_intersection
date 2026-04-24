add_library(qi_experiment STATIC
    version.cpp
    version.hpp
)
target_include_directories(qi_experiment PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qi_experiment
    PUBLIC
        Qt6::Core
        qi_geometry
        qi_mesh
        qi_triangulation
        qi_intersection
)
