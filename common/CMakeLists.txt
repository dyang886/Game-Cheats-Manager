add_library(Common INTERFACE)

# Set include directories for Common
target_include_directories(Common INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Define FLTK library directory based on architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64-bit architecture
    set(FLTK_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/x64")
    set(FLTK_LIBS
        "${FLTK_LIB_DIR}/fltk.lib"
        "${FLTK_LIB_DIR}/fltk_forms.lib"
        "${FLTK_LIB_DIR}/fltk_images.lib"
        "${FLTK_LIB_DIR}/fltk_png.lib"
        "${FLTK_LIB_DIR}/fltk_z.lib"
    )
    set(MONO_BRIDGE_DEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/x64")
else()
    # 32-bit architecture
    set(FLTK_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/x86")
    set(FLTK_LIBS
        "${FLTK_LIB_DIR}/fltk.lib"
        "${FLTK_LIB_DIR}/fltk_forms.lib"
        "${FLTK_LIB_DIR}/fltk_images.lib"
        "${FLTK_LIB_DIR}/fltk_png.lib"
        "${FLTK_LIB_DIR}/fltk_z.lib"
    )
    set(MONO_BRIDGE_DEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/x86")
endif()

# Link FLTK and Windows libraries to Common
target_link_libraries(Common INTERFACE
    ${FLTK_LIBS}
    comctl32
    gdiplus
    ws2_32
    shlwapi
)

# Define MonoBridge as a shared library
add_library(MonoBridge SHARED
    "${CMAKE_CURRENT_SOURCE_DIR}/include/MonoBridge.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/MonoBridge.def"
)

# Set properties for MonoBridge
set_target_properties(MonoBridge PROPERTIES
    OUTPUT_NAME "MonoBridge"
)

# Link MonoBridge to Common
target_link_libraries(MonoBridge PRIVATE Common)

# Post-build command to move only MonoBridge.dll to the destination
add_custom_command(TARGET MonoBridge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:MonoBridge>"
        "${MONO_BRIDGE_DEST_DIR}/MonoBridge.dll"
    COMMENT "Moving MonoBridge.dll to ${MONO_BRIDGE_DEST_DIR}"
)