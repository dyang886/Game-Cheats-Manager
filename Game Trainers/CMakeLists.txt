cmake_minimum_required(VERSION 3.15)

project(GameTrainers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddTrainer.cmake)

add_subdirectory(common)

# Determine current platform
if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CURRENT_PLATFORM "x64")
else()
    set(CURRENT_PLATFORM "x86")
endif()

message(STATUS "Building for platform: ${CURRENT_PLATFORM}")

file(GLOB TRAINER_CMAKE_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/trainers/*/CMakeLists.txt")
list(SORT TRAINER_CMAKE_FILES)

foreach(TRAINER_CMAKE_FILE ${TRAINER_CMAKE_FILES})
    get_filename_component(TRAINER_DIR "${TRAINER_CMAKE_FILE}" DIRECTORY)
    get_filename_component(TRAINER_FOLDER "${TRAINER_DIR}" NAME)

    file(READ "${TRAINER_CMAKE_FILE}" TRAINER_CMAKE_CONTENT)
    string(REGEX MATCH "set\\([ \t]*TRAINER_ARCH[ \t]+\"?([^\"\r\n\)]+)\"?[ \t]*\\)" TRAINER_ARCH_MATCH "${TRAINER_CMAKE_CONTENT}")

    if(TRAINER_ARCH_MATCH)
        set(TRAINER_ARCH "${CMAKE_MATCH_1}")
    elseif(TRAINER_CMAKE_CONTENT MATCHES "add_trainer")
        if(TRAINER_CMAKE_CONTENT MATCHES "ARCH[ 	]+\"?([A-Za-z0-9_]+)\"?")
            set(TRAINER_ARCH "${CMAKE_MATCH_1}")
        else()
            message(WARNING "Skipping ${TRAINER_FOLDER}: add_trainer() must declare ARCH")
            continue()
        endif()
    else()
        message(WARNING "Skipping ${TRAINER_FOLDER}: no add_trainer() call and no set(TRAINER_ARCH ...)")
        continue()
    endif()
    set(TRAINER_REQUIRED_ARCH "${TRAINER_ARCH}")

    if(TRAINER_ARCH STREQUAL CURRENT_PLATFORM)
        message(STATUS "Adding: ${TRAINER_FOLDER} (${TRAINER_ARCH})")
        add_subdirectory("${TRAINER_DIR}")
    else()
        message(STATUS "Skipping: ${TRAINER_FOLDER} (requires ${TRAINER_ARCH}, building for ${CURRENT_PLATFORM})")
    endif()
endforeach()
