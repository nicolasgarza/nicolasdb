cmake_minimum_required(VERSION 3.14)
project(nicolassql LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
enable_testing()

add_subdirectory(lexer)
add_subdirectory(ast)

