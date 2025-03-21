set(GAME_NAME DREDGE)

# Define the MonoBridge DLL
add_library(MonoBridge SHARED 
    ${CMAKE_SOURCE_DIR}/common/include/MonoBridge.cpp
    ${CMAKE_SOURCE_DIR}/common/include/MonoBridge.def
)

# Set properties for MonoBridge
set_target_properties(MonoBridge PROPERTIES 
    OUTPUT_NAME "MonoBridge"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Define C# compilation variables
set(CSHARP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.cs")
set(CSHARP_DLL "${CMAKE_BINARY_DIR}/bin/GCMInjection.dll")

# Custom command to compile GCMInjection.cs
add_custom_command(
    OUTPUT ${CSHARP_DLL}
    COMMAND "dotnet" build "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.csproj" -c Release -o "${CMAKE_BINARY_DIR}/bin"
    DEPENDS ${CSHARP_SOURCE} "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.csproj"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} # Ensure it runs in the source directory
    COMMENT "Compiling GCMInjection.cs to GCMInjection.dll"
    VERBATIM
)

# Custom target to ensure GCMInjection.dll is built
add_custom_target(GCMInjection ALL
    DEPENDS ${CSHARP_DLL}
)

# Define the executable
add_executable(${GAME_NAME} main.cpp)

# Link against the Common library
target_link_libraries(${GAME_NAME} PRIVATE Common)

# Add dependencies on MonoBridge and GCMInjection
add_dependencies(${GAME_NAME} MonoBridge GCMInjection)

# Set a custom output name for the trainer
set_target_properties(${GAME_NAME} PROPERTIES 
    WIN32_EXECUTABLE $<$<NOT:$<CONFIG:Debug>>:ON>
    OUTPUT_NAME "DREDGE Trainer"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if (WIN32)
    set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")
    target_sources(${GAME_NAME} PRIVATE ${RESOURCE_FILE})
    set_source_files_properties(${RESOURCE_FILE} PROPERTIES LANGUAGE RC)
endif()