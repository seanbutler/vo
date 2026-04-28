cmake_minimum_required(VERSION 3.16)
project(lang CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(lang
    src/main.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/interpreter/value.cpp
    src/interpreter/environment.cpp
    src/interpreter/interpreter.cpp
)

target_include_directories(lang PRIVATE src)
target_compile_options(lang PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(lang PRIVATE dl)

