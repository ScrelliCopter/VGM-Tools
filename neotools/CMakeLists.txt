add_executable(adpcm adpcm.c)
set_property(TARGET adpcm PROPERTY C_STANDARD 99)
target_compile_options(adpcm PRIVATE ${WARNINGS})
target_link_libraries(adpcm Common::wave $<$<C_COMPILER_ID:Clang,GNU>:m>)

add_executable(adpcmb adpcmb.c)
target_compile_options(adpcmb PRIVATE ${WARNINGS})
target_link_libraries(adpcmb Common::headers)

add_executable(neoadpcmextract autoextract.c neoadpcmextract.c)
set_property(TARGET neoadpcmextract PROPERTY C_STANDARD 99)
target_compile_options(neoadpcmextract PRIVATE ${WARNINGS})
