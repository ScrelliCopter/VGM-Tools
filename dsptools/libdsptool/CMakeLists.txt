option(DSPTOOL_BUILD_SHARED_LIBS "Build as a Shared Object or DLL" OFF)

set(HEADERS dsptool.h)
set(SOURCES math.c decode.c encode.c)

if (DSPTOOL_BUILD_SHARED_LIBS)
	add_library(DspTool SHARED ${HEADERS} ${SOURCES})
	target_compile_definitions(DspTool PRIVATE BUILD_SHARED)
else()
	add_library(DspTool STATIC ${HEADERS} ${SOURCES})
	target_compile_definitions(DspTool PRIVATE BUILD_STATIC)
endif()
add_library(DspTool::DspTool ALIAS DspTool)

set_property(TARGET DspTool PROPERTY C_STANDARD 99)
target_include_directories(DspTool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(DspTool PRIVATE ${WARNINGS})
target_link_libraries(DspTool PRIVATE Common::headers)
