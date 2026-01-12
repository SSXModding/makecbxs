# Main CMakeLists.txt
cmake_minimum_required(VERSION 3.15.0)
project(makecbxs CXX)

set(MAKECBXS_SOURCES
	src/main.cpp
	src/RefPack.cpp
	src/mmapfile.cpp
)

add_executable(makecbxs ${MAKECBXS_SOURCES})

target_include_directories(makecbxs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Enforce C++23 support
set_property(TARGET makecbxs PROPERTY CXX_STANDARD 23)
set_property(TARGET makecbxs PROPERTY CXX_STANDARD_REQUIRED ON)
