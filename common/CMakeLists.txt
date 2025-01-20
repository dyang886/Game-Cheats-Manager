# Specify the minimum CMake version and project name
cmake_minimum_required(VERSION 3.15)

add_library(Common INTERFACE)

# Include directories for the Common library
target_include_directories(Common INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(FLTK_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
target_link_libraries(${GAME_NAME} Common INTERFACE
    ${FLTK_LIB_DIR}/fltk.lib
    ${FLTK_LIB_DIR}/fltk_forms.lib
    ${FLTK_LIB_DIR}/fltk_images.lib
    ${FLTK_LIB_DIR}/fltk_jpeg.lib
)

target_link_libraries(Common INTERFACE comctl32 gdiplus ws2_32)
