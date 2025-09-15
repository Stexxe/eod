cmake_minimum_required(VERSION 3.16)
project(eod)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".js" CACHE INTERNAL "")
endif()

add_executable(eod main.c)

target_link_libraries(eod PRIVATE SDL3::SDL3)