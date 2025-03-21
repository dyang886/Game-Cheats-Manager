cmake_minimum_required(VERSION 3.15)

add_library(Common INTERFACE)

target_include_directories(Common INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# FLTK libraries based on architecture
if (CMAKE_SIZEOF_VOID_P EQUAL 8) # 64-bit
    set(FLTK_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/x64)
    target_link_libraries(Common INTERFACE 
        ${FLTK_LIB_DIR}/fltk.lib
        ${FLTK_LIB_DIR}/fltk_forms.lib
        ${FLTK_LIB_DIR}/fltk_images.lib
        ${FLTK_LIB_DIR}/fltk_jpeg.lib
    )
else() # 32-bit
    set(FLTK_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/x86)
    target_link_libraries(Common INTERFACE 
        ${FLTK_LIB_DIR}/fltk.lib
        ${FLTK_LIB_DIR}/fltk_forms.lib
        ${FLTK_LIB_DIR}/fltk_images.lib
        ${FLTK_LIB_DIR}/fltk_jpeg.lib
    )
endif()

target_link_libraries(Common INTERFACE comctl32 gdiplus ws2_32)
