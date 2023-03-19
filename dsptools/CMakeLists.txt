cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(dsptools LANGUAGES C)

include(CheckIncludeFile)
check_include_file("endian.h" HAVE_ENDIAN_H)
if (HAVE_ENDIAN_H)
	add_definitions(HAVE_ENDIAN_H)
endif()

add_subdirectory(libdsptool)

set(HEADERS wave.h)
set(SOURCES wavefile.c wave.c dspdecode.c)

add_executable(dspdecode ${HEADERS} ${SOURCES})

set_property(TARGET dspdecode PROPERTY C_STANDARD 99)
target_link_libraries(dspdecode DspTool::DspTool)
target_compile_options(dspdecode PRIVATE
	$<$<C_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -pedantic>
	$<$<C_COMPILER_ID:MSVC>:/Wall>)
