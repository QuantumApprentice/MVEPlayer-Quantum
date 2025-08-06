cmake_minimum_required(VERSION 3.25)
project(QMVEPlayer_Quantum LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

set(EXE_NAME "QMVEPlayer_Quantum")
add_executable(${EXE_NAME})
target_sources(
    ${EXE_NAME} PRIVATE
    main.cpp
)