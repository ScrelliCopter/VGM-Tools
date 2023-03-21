add_library(headers INTERFACE)
add_library(Common::headers ALIAS headers)
target_include_directories(headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(wave endian.h wave.h wave.c wavefile.c)
add_library(Common::wave ALIAS wave)
target_compile_options(wave PRIVATE ${WARNINGS})
target_link_libraries(wave PUBLIC headers)
