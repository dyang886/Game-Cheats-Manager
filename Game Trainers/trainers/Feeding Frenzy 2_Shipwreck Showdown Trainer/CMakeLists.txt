set(GAME_NAME "Feeding_Frenzy_2_Shipwreck_Showdown")
set(TRAINER_NAME "Feeding Frenzy 2_Shipwreck Showdown Trainer")

set(VENV_PYTHON "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe")
set(SUBSET_FONT_TARGET "${GAME_NAME}_SubsetFont")
set(FONT_SUBSET_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/NotoSansSC-Subset.ttf")
set(FONT_SUBSET_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/font_processor.py")
set(FONT_FILE "${CMAKE_SOURCE_DIR}/scripts/NotoSansSC-Regular.ttf")
set(TRANSLATION_FILES "${CMAKE_CURRENT_SOURCE_DIR}/translations.json")

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

add_executable(${GAME_NAME} main.cpp)
target_link_libraries(${GAME_NAME} PRIVATE Common)
add_dependencies(${GAME_NAME} ${SUBSET_FONT_TARGET})
set_target_properties(${GAME_NAME} PROPERTIES
    WIN32_EXECUTABLE $<$<NOT:$<CONFIG:Debug>>:ON>
    OUTPUT_NAME ${TRAINER_NAME}
)

if (WIN32)
    set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")
    target_sources(${GAME_NAME} PRIVATE ${RESOURCE_FILE})
    set_source_files_properties(${RESOURCE_FILE} PROPERTIES LANGUAGE RC)
endif()

# Move trainer executable to build/bin/
add_custom_command(TARGET ${GAME_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${GAME_NAME}>" "${CMAKE_BINARY_DIR}/bin/${TRAINER_NAME}/${TRAINER_NAME}.exe"
    COMMENT "Moving ${TRAINER_NAME}.exe to ${CMAKE_BINARY_DIR}/bin/${TRAINER_NAME}"
)