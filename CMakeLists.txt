# Minimum version used by CMake
cmake_minimum_required(VERSION 3.0)

# Project name
project(project_name)

# C++ version used, added compilation parameters
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -g -O0 -ggdb")

 
# Specify header file directory
include_directories(include)
 

# Find all source files in the current directory
aux_source_directory(. SOURCES)
 
# Compile all source code together and link it to an executable file 'a.out'
add_executable(a.out ${SOURCES})


