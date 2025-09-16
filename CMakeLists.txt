cmake_minimum_required(VERSION 3.16)
project(eod)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(vendor/SDL)

set(SDLTTF_VENDORED ON)
add_subdirectory(vendor/SDL_ttf)

add_executable(eod src/main.c src/mem.c)

set(ASSET_DIR "${CMAKE_SOURCE_DIR}/assets")
set_target_properties(eod PROPERTIES
    LINK_FLAGS "--preload-file ${ASSET_DIR}@/"
)

if(EMSCRIPTEN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sALLOW_MEMORY_GROWTH=1")
    set(CMAKE_EXECUTABLE_SUFFIX ".js" CACHE INTERNAL "")
endif()

target_link_libraries(eod PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)
#target_include_directories(eod PRIVATE /home/stexe/projects/emsdk/upstream/emscripten/system/include)