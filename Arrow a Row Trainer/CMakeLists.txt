set(GAME_NAME Arrow_A_Row)

# Define the executable
add_executable(${GAME_NAME} main.cpp)

# Link against the Common library
target_link_libraries(${GAME_NAME} PRIVATE Common)

# Set a custom output name (optional)
set_target_properties(${GAME_NAME} PROPERTIES OUTPUT_NAME "Arrow a Row Trainer")
