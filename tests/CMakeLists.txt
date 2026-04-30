find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(qi_tests
    geometry_tests.cpp
    mesh_tests.cpp
    triangulation_tests.cpp
)
target_link_libraries(qi_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        qi_geometry
        qi_mesh
        qi_triangulation
)
gtest_discover_tests(qi_tests)
