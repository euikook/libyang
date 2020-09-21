cmake_minimum_required (VERSION 2.6)
project (Main)

SET(CMAKE_CXX_COMPILER "/usr/bin/g++")
SET(CMAKE_C_COMPILER "/usr/bin/gcc")

SET(CMAKE_CXX_FLAGS "-Wall -Wextra")
SET(CMAKE_C_FLAGS "-Wall -Wextra -Wno-missing-field-initializers -std=c99 -g3 -O0 -fPIC -fvisibility=hidden")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(test_node_rel test_node_rel.cpp new.c)

