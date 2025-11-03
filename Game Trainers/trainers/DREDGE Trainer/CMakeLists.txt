set(GAME_NAME "DREDGE")
set(TRAINER_NAME "DREDGE Trainer")

set(CSHARP_TARGET "${GAME_NAME}_GCMInjection")
set(CSHARP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.cs")
set(CSHARP_DLL "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.dll")
set(CSHARP_TEMP_DIR "${CMAKE_BINARY_DIR}/trainers/${TRAINER_NAME}/GCMInjection")
set(CSHARP_TEMP_OUTPUT "${CSHARP_TEMP_DIR}/GCMInjection.dll")

set(VENV_PYTHON "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe")
set(SUBSET_FONT_TARGET "${GAME_NAME}_SubsetFont")
set(FONT_SUBSET_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/NotoSansSC-Subset.ttf")
set(FONT_SUBSET_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/font_processor.py")
set(FONT_FILE "${CMAKE_SOURCE_DIR}/scripts/NotoSansSC-Regular.ttf")
set(TRANSLATION_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/translations.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/translation_extra.txt"
)

# Run python script to generate the subsetted font
add_custom_command(
    OUTPUT ${FONT_SUBSET_OUTPUT}
    COMMAND ${VENV_PYTHON} ${FONT_SUBSET_SCRIPT} ${TRANSLATION_FILES}
            --font ${FONT_FILE} --output ${FONT_SUBSET_OUTPUT}
    DEPENDS ${FONT_SUBSET_SCRIPT} ${FONT_FILE} ${TRANSLATION_FILES}
    COMMENT "Subsetting NotoSansSC-Regular.ttf in ${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(${SUBSET_FONT_TARGET} ALL
    DEPENDS ${FONT_SUBSET_OUTPUT}
)

# Compile GCMInjection.cs to the build directory
add_custom_command(
    OUTPUT ${CSHARP_TEMP_OUTPUT}
    COMMAND "dotnet" build "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.csproj" -c Release -o "${CSHARP_TEMP_DIR}"
    DEPENDS ${CSHARP_SOURCE} "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.csproj"
    COMMENT "Compiling GCMInjection.cs to temporary output in ${CSHARP_TEMP_DIR}"
    VERBATIM
)

# Move GCMInjection.dll to the trainer folder
add_custom_command(
    OUTPUT ${CSHARP_DLL}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CSHARP_TEMP_OUTPUT}
        ${CSHARP_DLL}
    DEPENDS ${CSHARP_TEMP_OUTPUT}
    COMMENT "Moving GCMInjection.dll to ${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(${CSHARP_TARGET} ALL
    DEPENDS ${CSHARP_DLL}
)

add_executable(${GAME_NAME} main.cpp)
target_link_libraries(${GAME_NAME} PRIVATE Common)
add_dependencies(${GAME_NAME} ${SUBSET_FONT_TARGET} ${CSHARP_TARGET} MonoBridge)
set_target_properties(${GAME_NAME} PROPERTIES
    WIN32_EXECUTABLE $<$<NOT:$<CONFIG:Debug>>:ON>
    OUTPUT_NAME ${TRAINER_NAME}
)

# Handle Windows-specific resources file
if(WIN32)
    set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")
    target_sources(${GAME_NAME} PRIVATE ${RESOURCE_FILE})
    set_source_files_properties(${RESOURCE_FILE} PROPERTIES LANGUAGE RC)
endif()

# Move trainer executable to build/bin/ and embed manifest
add_custom_command(TARGET ${GAME_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${GAME_NAME}>" "${CMAKE_BINARY_DIR}/bin/${TRAINER_NAME}/${TRAINER_NAME}.exe"
    COMMAND "mt.exe" -manifest "${CMAKE_SOURCE_DIR}/common/assets/elevate.xml" "-outputresource:${CMAKE_BINARY_DIR}/bin/${TRAINER_NAME}/${TRAINER_NAME}.exe;1"
    COMMENT "Moving ${TRAINER_NAME}.exe to ${CMAKE_BINARY_DIR}/bin/${TRAINER_NAME} and embedding manifest"
)