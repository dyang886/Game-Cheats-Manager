set(GAME_NAME "PvZ2_Gardendless")
set(TRAINER_NAME "PvZ2 Gardendless Trainer")

set(VENV_PYTHON "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe")
set(SUBSET_FONT_TARGET "${GAME_NAME}_SubsetFont")
set(FONT_SUBSET_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/NotoSansSC-Subset.ttf")
set(FONT_SUBSET_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/font_processor.py")
set(FONT_FILE "${CMAKE_SOURCE_DIR}/scripts/NotoSansSC-Regular.ttf")
set(TRANSLATION_FILES "${CMAKE_CURRENT_SOURCE_DIR}/translations.json")

add_custom_command(
    OUTPUT ${FONT_SUBSET_OUTPUT}
    COMMAND ${VENV_PYTHON} ${FONT_SUBSET_SCRIPT} ${TRANSLATION_FILES}
            --font ${FONT_FILE} --output ${FONT_SUBSET_OUTPUT}
    DEPENDS ${FONT_SUBSET_SCRIPT} ${FONT_FILE} ${TRANSLATION_FILES}
    COMMENT "Subsetting NotoSansSC-Regular.ttf in ${CMAKE_CURRENT_SOURCE_DIR}"
)
add_custom_target(${SUBSET_FONT_TARGET} ALL DEPENDS ${FONT_SUBSET_OUTPUT})

# Main executable
add_executable(${GAME_NAME} main.cpp)

# Link Common (FLTK + Windows libs) plus winhttp for CDP WebSocket
target_link_libraries(${GAME_NAME} PRIVATE Common winhttp ole32 comdlg32 ws2_32)
add_dependencies(${GAME_NAME} ${SUBSET_FONT_TARGET})

set_target_properties(${GAME_NAME} PROPERTIES
    WIN32_EXECUTABLE $<$<NOT:$<CONFIG:Debug>>:ON>
    OUTPUT_NAME "${TRAINER_NAME}"
)

# Embed Windows resources (icon, logo, translations, font)
if (WIN32)
    set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")
    target_sources(${GAME_NAME} PRIVATE ${RESOURCE_FILE})
    set_source_files_properties(${RESOURCE_FILE} PROPERTIES LANGUAGE RC)
endif()

# Copy to bin/ output directory
add_custom_command(TARGET ${GAME_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${GAME_NAME}>"
        "${CMAKE_BINARY_DIR}/bin/${TRAINER_NAME}/${TRAINER_NAME}.exe"
    COMMENT "Moving ${TRAINER_NAME}.exe to ${CMAKE_BINARY_DIR}/bin/${TRAINER_NAME}"
)
