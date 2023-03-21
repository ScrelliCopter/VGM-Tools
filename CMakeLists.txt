cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(VGM-Tools LANGUAGES C)

set(WARNINGS
	$<$<C_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -pedantic -Wno-unused-parameter>
	$<$<C_COMPILER_ID:MSVC>:/Wall /wd4100>)

add_subdirectory(common)
add_subdirectory(dsptools)
add_subdirectory(neotools)
