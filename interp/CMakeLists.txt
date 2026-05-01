cmake_minimum_required(VERSION 3.16)
project(vo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(vo
    src/main.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/interpreter/value.cpp
    src/interpreter/environment.cpp
    src/interpreter/interpreter.cpp
)

target_include_directories(vo PRIVATE src)
target_compile_options(vo PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(vo PRIVATE dl)
set_target_properties(vo PROPERTIES BUILD_RPATH "$ORIGIN")

set(VTK_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(VTK_BUILD_STATIC  OFF CACHE BOOL "" FORCE)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_subdirectory(vendor/vtkit)

