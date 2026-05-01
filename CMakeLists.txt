cmake_minimum_required(VERSION 3.20)
project(quadric_intersections
    VERSION 1.0.0
    DESCRIPTION "Quadric surface triangulation and pairwise intersection viewer"
    LANGUAGES CXX
)

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

# ---- install + packaging ----
include(GNUInstallDirs)

install(TARGETS quadric_intersections
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})

set(CPACK_PACKAGE_NAME "quadric-intersections")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VENDOR "qi project")
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)
