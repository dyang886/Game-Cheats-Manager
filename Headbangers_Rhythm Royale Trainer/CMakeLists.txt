# Use static runtime library for release builds
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(RESOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources.rc)
set(FREETYPE_LIB_DIR ${CMAKE_SOURCE_DIR}/common/libs)

# Add the executable for this trainer
add_executable(HeadbangersTrainer WIN32 main.cpp)

# Include the shared headers
target_include_directories(HeadbangersTrainer PRIVATE ${CMAKE_SOURCE_DIR}/common/include)

# Link the common library
target_link_libraries(HeadbangersTrainer PRIVATE Common d3d12 dxgi dxguid)
target_link_libraries(HeadbangersTrainer PRIVATE ${FREETYPE_LIB_DIR}/freetype_x64.lib)

target_sources(HeadbangersTrainer PRIVATE ${RESOURCE_FILE})

# Set custom output name
set_target_properties(HeadbangersTrainer PROPERTIES OUTPUT_NAME "Headbangers_Rhythm Royale Trainer")
