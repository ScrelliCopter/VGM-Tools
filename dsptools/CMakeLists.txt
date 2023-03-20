cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(dsptools LANGUAGES C)

set(COMMON "${CMAKE_SOURCE_DIR}/../common")

add_subdirectory(libdsptool)

set(HEADERS ${COMMON}/wave.h ${COMMON}/endian.h)
set(SOURCES ${COMMON}/wavefile.c ${COMMON}/wave.c dspdecode.c)

add_executable(dspdecode ${HEADERS} ${SOURCES})

set_property(TARGET dspdecode PROPERTY C_STANDARD 99)
target_include_directories(dspdecode PRIVATE ${COMMON})
target_link_libraries(dspdecode DspTool::DspTool)
target_compile_options(dspdecode PRIVATE
	$<$<C_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -pedantic>
	$<$<C_COMPILER_ID:MSVC>:/Wall>)
