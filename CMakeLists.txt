cmake_minimum_required(VERSION 3.25)
project(QMVEPlayer_Quantum LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

set(EXE_NAME "QMVEPlayer_Quantum")
add_executable(${EXE_NAME})
target_sources(
    ${EXE_NAME} PRIVATE
    main.cpp
    parse_video.cpp
    parse_audio.cpp
    parse_opcodes.cpp
)

add_library(ImGui
    dependencies/imgui-1.92.1/backends/imgui_impl_opengl3.h
    dependencies/imgui-1.92.1/backends/imgui_impl_opengl3.cpp
    dependencies/imgui-1.92.1/backends/imgui_impl_glfw.h
    dependencies/imgui-1.92.1/backends/imgui_impl_glfw.cpp
    dependencies/imgui-1.92.1/imgui.h
    dependencies/imgui-1.92.1/imgui.cpp
    dependencies/imgui-1.92.1/imgui_demo.cpp
    dependencies/imgui-1.92.1/imgui_draw.cpp
    dependencies/imgui-1.92.1/imgui_tables.cpp
    dependencies/imgui-1.92.1/imgui_widgets.cpp
)

add_subdirectory(
    dependencies/glfw-3.4/              SYSTEM
)
add_subdirectory(
    dependencies/glad1/                 SYSTEM
    # dependencies/glad-2.0.8/
)

target_include_directories(
    ImGui PUBLIC
    dependencies/glfw-3.4/include/
    dependencies/imgui-1.92.1/
    dependencies/imgui-1.92.1/backends/
)

target_link_libraries(
    ${EXE_NAME} PRIVATE
    ImGui
    glad
    glfw
)