find_package(ZLIB)
option(USE_ZLIB "Link Zlib for VGZ support" ${ZLIB_FOUND})
if (USE_ZLIB AND NOT ZLIB_FOUND)
	message(FATAL_ERROR "USE_ZLIB specified but Zlib was not found")
endif()

add_executable(adpcm adpcm.h libadpcma.c adpcm.c)
set_property(TARGET adpcm PROPERTY C_STANDARD 99)
target_compile_options(adpcm PRIVATE ${WARNINGS})
target_link_libraries(adpcm Common::wave $<$<C_COMPILER_ID:Clang,GNU>:m>)

add_executable(adpcmb adpcmb.h libadpcmb.c adpcmb.c)
set_property(TARGET adpcmb PROPERTY C_STANDARD 99)
target_compile_options(adpcmb PRIVATE ${WARNINGS})
target_link_libraries(adpcmb Common::wave)

add_executable(neoadpcmextract
	gzstreamfile.c
	libadpcma.c adpcm.h
	libadpcmb.c adpcmb.h
	neoadpcmextract.c)
set_property(TARGET neoadpcmextract PROPERTY C_STANDARD 99)
target_compile_definitions(neoadpcmextract PRIVATE $<$<BOOL:${USE_ZLIB}>:USE_ZLIB=1>)
target_compile_options(neoadpcmextract PRIVATE ${WARNINGS})
target_link_libraries(neoadpcmextract $<$<BOOL:${USE_ZLIB}>:ZLIB::ZLIB> Common::wave)
