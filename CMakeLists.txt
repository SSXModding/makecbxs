# Main CMakeLists.txt

cmake_minimum_required(VERSION 3.1.0)
project(makecbxs CXX)
find_package(Threads)


if(CMAKE_VERSION VERSION_LESS "3.7.0")
	set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

if(MSVC)
	# MSVC tends to fake C++98 even with /std:c++17 on the command line..
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /D_CRT_SECURE_NO_WARNINGS")
endif()

# clang-cl optimizations
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND MSVC)
	if("${CMAKE_BUILD_TYPE}" MATCHES "Release")
		set(CMAKE_CXX_FLAGS_RELEASE "/MD /Oi /DNDEBUG /clang:-O3 /clang:-ffast-math")
		set(CMAKE_C_FLAGS_RELEASE "/MD /Oi /DNDEBUG /clang:-O3 /clang:-ffast-math")
	endif()
elseif(MSVC)
	# not clang-cl, but instead, it is *really* MSVC
endif()


set(MCOMMON_DO_NOT_INSTALL ON)
add_subdirectory(vendor/mcommon)

set(MAKECBXS_SOURCES
	main.cpp
	RefPack.cpp
)

add_executable(makecbxs ${MAKECBXS_SOURCES})

if(MAKECBXS_STDLIB STREQUAL "llvm")
	message(STATUS "Linking c++fs library")
	target_link_libraries(makecbxs c++fs)
elseif(MAKECBXS_STDLIB STREQUAL "gnu")
	message(STATUS "Linking stdc++fs library")
	target_link_libraries(makecbxs stdc++fs)
endif()


target_include_directories(makecbxs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Enforce C++17 support
set_property(TARGET makecbxs PROPERTY CXX_STANDARD 17)
set_property(TARGET makecbxs PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(makecbxs mcommon)