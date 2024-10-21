# Minimum version used by CMake
cmake_minimum_required(VERSION 3.10)
# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project name
project(project_name)

# C++ version used, added compilation parameters
set(CMAKE_CXX_STANDARD 14)

IF (LINUX)
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -O0 -Wno-class-memaccess -D SHOW_MB_BORDER")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -g3 -O0 -Wno-class-memaccess")
ELSEIF (APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-system-headers -Wall -g3 -O0 -isystem /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1 -isystem /Library/Developer/CommandLineTools/usr/lib/clang/15.0.0/include")
ENDIF()

# Specify header file directory
include_directories(include)

# Find all source files in the current directory
aux_source_directory(. SOURCES)

# Compile all source code together and link it to an executable file 'a.out'
add_executable(a.out ${SOURCES})


