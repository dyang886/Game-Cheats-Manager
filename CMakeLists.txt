cmake_minimum_required(VERSION 3.15)

project(GameTrainers LANGUAGES CXX)

# Set C++ standard and runtime library
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(common)

# Add each game-specific trainer
set(TRAINER_DIRS
    "trainers/Arrow a Row Trainer"
    "trainers/DREDGE Trainer"
    "trainers/Feeding Frenzy 2_Shipwreck Showdown Trainer"
    "trainers/Headbangers_Rhythm Royale Trainer"
    "trainers/Heavy Weapon Trainer"
    "trainers/Plants vs. Zombies Trainer"
    "trainers/Wizard of Legend 2 Trainer"
)

foreach(TRAINER_DIR ${TRAINER_DIRS})
    add_subdirectory(${TRAINER_DIR})
endforeach()