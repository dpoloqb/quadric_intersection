add_library(qi_mesh STATIC
    version.cpp
    version.hpp
)
target_include_directories(qi_mesh PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qi_mesh PUBLIC Eigen3::Eigen)
