cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(DspTool LANGUAGES C)

option(BUILD_SHARED_LIBS "Build as a Shared Object or DLL" OFF)

set(HEADERS dsptool.h ${COMMON}/util.h)
set(SOURCES math.c decode.c encode.c)

add_library(DspTool ${HEADERS} ${SOURCES})
add_library(DspTool::DspTool ALIAS DspTool)

set_property(TARGET DspTool PROPERTY C_STANDARD 99)
target_include_directories(DspTool
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${COMMON})
if (BUILD_SHARED_LIBS)
	target_compile_definitions(DspTool PRIVATE BUILD_SHARED)
endif()
target_compile_options(DspTool PRIVATE
	$<$<C_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -pedantic -Wno-unused-parameter>
	$<$<C_COMPILER_ID:MSVC>:/Wall /wd4100>)
