cmake_minimum_required(VERSION 3.14)
project(neo2_llhk)

set(CMAKE_CXX_STANDARD 20)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(neo2_llhk main.cpp)