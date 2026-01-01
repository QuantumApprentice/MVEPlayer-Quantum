cmake_minimum_required(VERSION 3.25)
project(QMVEPlayer_Quantum LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

set(EXE_NAME "QMVEPlayer_Quantum")
add_executable(${EXE_NAME})
target_sources(
    ${EXE_NAME} PRIVATE
    src/main.cpp
    src/parse_video.cpp
    src/parse_audio.cpp
    src/parse_audio_alsa.cpp
    # src/parse_audio_pipewire.cpp
    src/parse_audio_pipewire_thread.cpp
    src/parse_audio_sdl.cpp
    src/parse_opcodes.cpp
    src/MiniSDL.cpp
    src/shader_class.cpp
    src/io_OpenGL.cpp
    src/io_Platform.cpp
    src/io_Timer.cpp
    src/ring_buffer.cpp
)

# Windows compile stuff
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
target_compile_definitions(
  ${EXE_NAME}
  PUBLIC WINDOWS
)

# Linux compile stuff
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
target_compile_definitions(
  ${EXE_NAME}
  PUBLIC LINUX
)

find_package(ALSA)
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE ALSA::ALSA
    PRIVATE SDL2::SDL2
    PRIVATE SDL2_mixer::SDL2_mixer
)

find_package(PkgConfig)
if (PkgConfig_FOUND)
    # pkg_search_module(Pipewire libpipewire-0.3 REQUIRED)
    pkg_check_modules(Pipewire IMPORTED_TARGET libpipewire-0.3 REQUIRED)
    if (Pipewire_FOUND)
        target_link_libraries(${PROJECT_NAME}
            PUBLIC PkgConfig::Pipewire
        )
    endif()

    pkg_check_modules(Pipewire libpipewire-0.3 REQUIRED)
endif()

# pipewire compiler command arguments that ultimately gets run:
#     -I/usr/include/pipewire-0.3 -I/usr/include/spa-0.2 -D_REENTRANT -lpipewire-0.3
endif()



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