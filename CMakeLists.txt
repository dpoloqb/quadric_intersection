cmake_minimum_required(VERSION 3.20)
project(quadric_intersections LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(QI_BUILD_TESTS "Build unit tests" ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Sql OpenGLWidgets Test Concurrent)
find_package(Eigen3 CONFIG REQUIRED)

enable_testing()

add_subdirectory(src/geometry)
add_subdirectory(src/mesh)
add_subdirectory(src/triangulation)
add_subdirectory(src/intersection)
add_subdirectory(src/storage)
add_subdirectory(src/experiment)
add_subdirectory(src/ui)

qt_standard_project_setup()

add_executable(quadric_intersections src/main.cpp)
target_link_libraries(quadric_intersections PRIVATE qi_ui Qt6::Widgets)

if(QI_BUILD_TESTS)
    add_subdirectory(tests)
endif()
