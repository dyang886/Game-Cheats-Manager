cmake_minimum_required(VERSION 3.15)

project(GameTrainers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Trainer architecture requirements (format: "TrainerName:architecture")
# Used by both CMakeLists.txt and build_all.ps1 (which parses this file)
set(TRAINER_ARCHITECTURES
    "Arrow a Row Trainer:x64"
    "DREDGE Trainer:x86"
    "Feeding Frenzy 2_Shipwreck Showdown Trainer:x86"
    "Headbangers_Rhythm Royale Trainer:x64"
    "Heavy Weapon Trainer:x86"
    "Inotia 4 Trainer:x64"
    "Just Shapes & Beats Trainer:x64"
    "Plants vs. Zombies_GOTY Edition Trainer:x86"
    "Plants vs. Zombies_Replanted Trainer:x64"
    "Wizard of Legend 2 Trainer:x64"
)

add_subdirectory(common)

# Determine current platform
if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CURRENT_PLATFORM "x64")
else()
    set(CURRENT_PLATFORM "x86")
endif()

message(STATUS "Building for platform: ${CURRENT_PLATFORM}")

# Add each game-specific trainer with architecture information
foreach(TRAINER_SPEC ${TRAINER_ARCHITECTURES})
    string(REPLACE ":" ";" TRAINER_PARTS ${TRAINER_SPEC})
    list(GET TRAINER_PARTS 0 TRAINER_NAME)
    list(GET TRAINER_PARTS 1 TRAINER_ARCH)
    
    set(TRAINER_DIR "trainers/${TRAINER_NAME}")
    set(TRAINER_REQUIRED_ARCH ${TRAINER_ARCH})
    
    # Only add trainers matching the current platform
    if(TRAINER_ARCH STREQUAL CURRENT_PLATFORM)
        message(STATUS "Adding: ${TRAINER_NAME} (${TRAINER_ARCH})")
        add_subdirectory(${TRAINER_DIR})
    else()
        message(STATUS "Skipping: ${TRAINER_NAME} (requires ${TRAINER_ARCH}, building for ${CURRENT_PLATFORM})")
    endif()
endforeach()