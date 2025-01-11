# Specify the minimum CMake version and project name
cmake_minimum_required(VERSION 3.15)

# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# file(GLOB COMMON_SOURCES "src/*.cpp")
# add_library(Common STATIC ${COMMON_SOURCES})

add_library(Common INTERFACE)

# Include directories for the Common library
target_include_directories(Common INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts
)

# Specify the path to FLTK libraries
set(FLTK_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

# Link FLTK libraries (adjust filenames as per your FLTK build)
target_link_libraries(Common INTERFACE 
    ${FLTK_LIB_DIR}/fltk_x64.lib
    ${FLTK_LIB_DIR}/fltk_forms_x64.lib
    ${FLTK_LIB_DIR}/fltk_images_x64.lib
    ${FLTK_LIB_DIR}/fltk_jpeg_x64.lib

    # ${FLTK_LIB_DIR}/fltk_x86.lib
    # ${FLTK_LIB_DIR}/fltk_forms_x86.lib
    # ${FLTK_LIB_DIR}/fltk_images_x86.lib
    # ${FLTK_LIB_DIR}/fltk_jpeg_x86.lib

    # ${FLTK_LIB_DIR}/fltk_png.lib
)

target_link_libraries(Common INTERFACE comctl32 gdiplus ws2_32)
