set(GAME_NAME "Arrow_A_Row")
set(TRAINER_NAME "Arrow A Row Trainer")


add_executable(${GAME_NAME} main.cpp)
target_link_libraries(${GAME_NAME} PRIVATE Common)
set_target_properties(${GAME_NAME} PROPERTIES
    WIN32_EXECUTABLE $<$<NOT:$<CONFIG:Debug>>:ON>
    OUTPUT_NAME "${TRAINER_NAME}"
)

if (WIN32)
    set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")
    target_sources(${GAME_NAME} PRIVATE ${RESOURCE_FILE})
    set_source_files_properties(${RESOURCE_FILE} PROPERTIES LANGUAGE RC)
endif()

# Move trainer executable to build/bin/
add_custom_command(TARGET ${GAME_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${GAME_NAME}>" "${CMAKE_BINARY_DIR}/bin/${TRAINER_NAME}.exe"
    COMMENT "Moving ${TRAINER_NAME}.exe to ${CMAKE_BINARY_DIR}/bin"
)