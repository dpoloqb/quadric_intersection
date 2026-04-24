add_library(qi_storage STATIC
    version.cpp
    version.hpp
)
target_include_directories(qi_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qi_storage
    PUBLIC
        Qt6::Core
        Qt6::Sql
)
