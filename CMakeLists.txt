# Minimum version used by CMake
cmake_minimum_required(VERSION 3.10)
# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# NOTE: Arm是为了在qemu中执行，所以编译为静态库、静态链接程序，X86为动态库、动态链接程序

# Project name
project(project_name)

# C++ version used, added compilation parameters
set(CMAKE_CXX_STANDARD 14)

#cmake . -B build -D BUILD_FOR_ARM=ON
if(BUILD_FOR_ARM)
	message(STATUS "Building for ARM64")
	set(CMAKE_SYSTEM_NAME Linux)
	set(CMAKE_SYSTEM_PROCESSOR aarch64)
	set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
	set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
	set(CMAKE_C_FLAGS "-march=armv8-a")
	set(CMAKE_CXX_FLAGS "-march=armv8-a")
endif()

IF (LINUX)
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -O0") #Debug
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -D DISABLE_COUT") #Release

	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -O3 -Wno-class-memaccess -D SHOW_MB_BORDER") #开启宏块分割线条显示
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -O0 -Wno-class-memaccess -D DISABLE_COUT") #关闭信息输出
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -O0 -Wno-class-memaccess -D SKIP_MB_DECODE") #跳过宏块解码
ELSEIF (APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-system-headers -Wall -g3 -O0 -isystem /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1 -isystem /Library/Developer/CommandLineTools/usr/lib/clang/15.0.0/include")
ENDIF()

# Specify header file directory
include_directories(include)

# Find all source files in the current directory
aux_source_directory(. SOURCES)
list(REMOVE_ITEM SOURCES main.cpp)

if(BUILD_FOR_ARM)
	add_library(h264decoder STATIC ${SOURCES})
else()
	add_library(h264decoder SHARED ${SOURCES})
endif()

# Compile all source code together and link it to an executable file 'a.out'
add_executable(a.out main.cpp)

if(BUILD_FOR_ARM)
	set_target_properties(a.out PROPERTIES LINK_FLAGS "-static")
endif()

target_link_libraries(a.out h264decoder)
