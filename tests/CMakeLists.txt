find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(qi_tests
    geometry_tests.cpp
)
target_link_libraries(qi_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        qi_geometry
)
gtest_discover_tests(qi_tests)
