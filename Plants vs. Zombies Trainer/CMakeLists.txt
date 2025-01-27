set(GAME_NAME PVZ)

# Define the executable
add_executable(${GAME_NAME} main.cpp)

# Link against the Common library
target_link_libraries(${GAME_NAME} PRIVATE Common)

# Set a custom output name
set_target_properties(${GAME_NAME} PROPERTIES 
    WIN32_EXECUTABLE $<$<NOT:$<CONFIG:Debug>>:ON>
    OUTPUT_NAME "Plants vs. Zombies Trainer"
)

if (WIN32)
    set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")
    target_sources(${GAME_NAME} PRIVATE ${RESOURCE_FILE})
    set_source_files_properties(${RESOURCE_FILE} PROPERTIES LANGUAGE RC)
endif()
