project(neoadpcmtools LANGUAGES C)
cmake_minimum_required(VERSION "3.1" FATAL_ERROR)

set(COMMON "${CMAKE_SOURCE_DIR}/../common")
set(WARNINGS
	$<$<C_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -pedantic -Wno-unused-parameter>
	$<$<C_COMPILER_ID:MSVC>:/Wall /wd4100>)

add_executable(adpcm ${COMMON}/util.h ${COMMON}/wave.h ${COMMON}/wave.c ${COMMON}/wavefile.c adpcm.c)
set_property(TARGET adpcm PROPERTY C_STANDARD 99)
target_compile_options(adpcm PRIVATE ${WARNINGS})
target_include_directories(adpcm PRIVATE ${COMMON})
target_link_libraries(adpcm $<$<C_COMPILER_ID:Clang,GNU>:m>)

add_executable(adpcmb ${COMMON}/util.h adpcmb.c)
target_compile_options(adpcmb PRIVATE ${WARNINGS})
target_include_directories(adpcmb PRIVATE ${COMMON})

add_executable(neoadpcmextract autoextract.c neoadpcmextract.c)
set_property(TARGET neoadpcmextract PROPERTY C_STANDARD 99)
target_compile_options(neoadpcmextract PRIVATE ${WARNINGS})
