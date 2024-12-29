cmake_minimum_required(VERSION 3.15)
project(GameTrainers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Add the common library
add_subdirectory(common)

# Add each game-specific trainer
add_subdirectory("Headbangers_Rhythm Royale Trainer")
