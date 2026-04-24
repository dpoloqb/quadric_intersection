add_library(qi_ui STATIC
    version.cpp
    version.hpp
    MainWindow.cpp
    MainWindow.hpp
    MainWindow.ui
)
target_include_directories(qi_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qi_ui
    PUBLIC
        Qt6::Widgets
        Qt6::OpenGLWidgets
)
