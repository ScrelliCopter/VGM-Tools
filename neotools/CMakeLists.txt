project(neoadpcmtools)
cmake_minimum_required(VERSION "3.1" FATAL_ERROR)

add_executable(adpcm adpcm.c)
target_compile_options(adpcm PRIVATE
	-fomit-frame-pointer -Werror -Wall
	-W -Wno-sign-compare -Wno-unused
	-Wpointer-arith -Wbad-function-cast -Wcast-align -Waggregate-return
	-pedantic
	-Wshadow
	-Wstrict-prototypes)
target_link_libraries(adpcm m)

add_executable(adpcmb adpcmb.c)

add_executable(neoadpcmextract autoextract.c neoadpcmextract.c)
set_property(TARGET neoadpcmextract PROPERTY C_STANDARD 99)
target_compile_options(neoadpcmextract PRIVATE -Wall -Wextra -pedantic)
