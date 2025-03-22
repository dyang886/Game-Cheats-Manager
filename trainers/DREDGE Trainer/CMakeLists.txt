set(GAME_NAME "DREDGE")
set(TRAINER_NAME "DREDGE Trainer")

set(CSHARP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.cs")
set(CSHARP_DLL "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.dll")
set(CSHARP_TEMP_DIR "${CMAKE_BINARY_DIR}/trainers/${TRAINER_NAME}/GCMInjection")
set(CSHARP_TEMP_OUTPUT "${CSHARP_TEMP_DIR}/GCMInjection.dll")

# Compile GCMInjection.cs to the build directory
add_custom_command(
    OUTPUT ${CSHARP_TEMP_OUTPUT}
    COMMAND "dotnet" build "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.csproj" -c Release -o "${CSHARP_TEMP_DIR}"
    DEPENDS ${CSHARP_SOURCE} "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.csproj"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Compiling GCMInjection.cs to temporary output in ${CSHARP_TEMP_DIR}"
    VERBATIM
)

# Move GCMInjection.dll to the trainer folder
add_custom_command(
    OUTPUT ${CSHARP_DLL}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CSHARP_TEMP_OUTPUT}"
        "${CSHARP_DLL}"
    DEPENDS ${CSHARP_TEMP_OUTPUT}
    COMMENT "Moving GCMInjection.dll to ${CMAKE_CURRENT_SOURCE_DIR}"
)

# Custom target to ensure GCMInjection.dll is built and moved
add_custom_target(GCMInjection ALL
    DEPENDS ${CSHARP_DLL}
)

add_executable(${GAME_NAME} main.cpp)
target_link_libraries(${GAME_NAME} PRIVATE Common)
add_dependencies(${GAME_NAME} GCMInjection MonoBridge)
set_target_properties(${GAME_NAME} PROPERTIES
    WIN32_EXECUTABLE $<$<NOT:$<CONFIG:Debug>>:ON>
    OUTPUT_NAME "${TRAINER_NAME}"
)

# Handle Windows-specific resources file
if(WIN32)
    set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")
    target_sources(${GAME_NAME} PRIVATE ${RESOURCE_FILE})
    set_source_files_properties(${RESOURCE_FILE} PROPERTIES LANGUAGE RC)
endif()

# Move trainer executable to build/bin/ and embed manifest
add_custom_command(TARGET ${GAME_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${GAME_NAME}>" "${CMAKE_BINARY_DIR}/bin/${TRAINER_NAME}.exe"
    COMMAND "mt.exe" -manifest "${CMAKE_SOURCE_DIR}/common/assets/elevate.xml" "-outputresource:${CMAKE_BINARY_DIR}/bin/${TRAINER_NAME}.exe;1"
    COMMENT "Moving ${TRAINER_NAME}.exe to ${CMAKE_BINARY_DIR}/bin and embedding manifest"
)