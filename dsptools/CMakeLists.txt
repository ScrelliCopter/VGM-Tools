add_subdirectory(libdsptool)

add_executable(dspdecode dspdecode.c)
set_property(TARGET dspdecode PROPERTY C_STANDARD 99)
target_include_directories(dspdecode PRIVATE ${COMMON})
target_link_libraries(dspdecode Common::wave DspTool::DspTool)
target_compile_options(dspdecode PRIVATE ${WARNINGS})
