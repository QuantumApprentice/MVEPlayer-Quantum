cmake_minimum_required(VERSION 3.25)
project(QMVEPlayer_TEST LANGUAGES CXX)
set(EXE_NAME "QMVEPlayer_TEST")

add_executable(
    ${EXE_NAME}
    video_tests.cpp
    ${CMAKE_SOURCE_DIR}/../io_OpenGL.cpp
    ${CMAKE_SOURCE_DIR}/../MiniSDL.cpp
    ${CMAKE_SOURCE_DIR}/../parse_audio.cpp
    ${CMAKE_SOURCE_DIR}/../parse_opcodes.cpp
    ${CMAKE_SOURCE_DIR}/../parse_video.cpp
    ${CMAKE_SOURCE_DIR}/../shader_class.cpp
)

add_subdirectory(
    ${CMAKE_SOURCE_DIR}/../dependencies/glad1/
    glad/
)

target_link_libraries(${EXE_NAME}
    glad
)

target_include_directories(
    ${EXE_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/../dependencies/glad1/include/
)

# add_test(
#     NAME QMVEPlayer_test
#     COMMAND QMVEPlayer_test
# )