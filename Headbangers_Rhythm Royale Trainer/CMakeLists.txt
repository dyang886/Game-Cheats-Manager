# Use static runtime library for release builds
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Add the executable for this trainer
add_executable(HeadbangersTrainer WIN32 main.cpp)

# Include the shared headers
target_include_directories(HeadbangersTrainer PRIVATE ${CMAKE_SOURCE_DIR}/common/include)

# Link the common library
target_link_libraries(HeadbangersTrainer PRIVATE Common d3d12 dxgi dxguid)

# Set custom output name
set_target_properties(HeadbangersTrainer PROPERTIES OUTPUT_NAME "Headbangers_Rhythm Royale Trainer")
