cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(DspTool LANGUAGES C)

set(CMAKE_C_STANDARD 99)

set(HEADERS dsptool.h)
set(SOURCES math.c decode.c encode.c)

add_library(DspTool ${HEADERS} ${SOURCES})
