cmake_minimum_required(VERSION 3.16)

project(DijkstraSimulator
    VERSION 2.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(DijkstraSimulator
    resources.qrc
    main.cpp
    MainWindow.cpp
    MainWindow.h
    GraphWidget.cpp
    GraphWidget.h
    PathPreview.cpp
    PathPreview.h
    DijkstraSimulator.cpp
    DijkstraSimulator.h
)

target_link_libraries(DijkstraSimulator
    PRIVATE Qt6::Widgets
)

if (WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS "${Qt6_DIR}/../../../bin"
    )
    if (WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET DijkstraSimulator POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                "$<TARGET_FILE:DijkstraSimulator>"
            COMMENT "Preparing Qt libraries for running from CLion"
            VERBATIM
        )
    else()
        message(WARNING "windeployqt was not found. Add the Qt bin directory to the CLion Run configuration PATH.")
    endif()
endif()

if (MSVC)
    target_compile_options(DijkstraSimulator PRIVATE /W4)
else()
    target_compile_options(DijkstraSimulator PRIVATE -Wall -Wextra -Wpedantic)
endif()
