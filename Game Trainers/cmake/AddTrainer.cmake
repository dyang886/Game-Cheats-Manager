# add_trainer() builds one trainer out of the files sitting in its own folder and generates the
# resource script that embeds them. The call is the trainer's manifest: everything about it that is
# not a cheat is declared here, and main.cpp is left holding nothing but the option list.
#
#   add_trainer(
#       ARCH            x64
#       RUNTIME         mono
#       GAME_VERSION    "1.1.1"
#       TRAINER_VERSION "1.0"
#       WINDOW          800 600
#       COLUMN_GAPS     80
#       COLUMN_ALIGN    center
#       ROW_GAP         8
#       INFO_ICON
#   )
#
# Everything above is required. Nothing defaults, on purpose: a default would let a trainer inherit
# a setting without saying so, and the manifest would stop describing the trainer completely.
#
#   ARCH             x64 or x86; a property of the game, not the trainer
#   RUNTIME          how the trainer reaches the game - see below
#   GAME_VERSION     version of the game this trainer was built against
#   TRAINER_VERSION  version of the trainer itself
#   WINDOW           window width and height
#   COLUMN_GAPS      one value per column of options, so the number of values is the number of
#                    columns. The first is the gap between the cover image and the first column,
#                    and each one after it is the gap before the column it belongs to.
#   COLUMN_ALIGN     top or center. One value for the whole window; per-column would look broken.
#                    register_cheats() picks a column with ui.column(n), defaulting to the first.
#   ROW_GAP          vertical space between one option row and the next.
#
# Optional, and meaningful by their absence:
#
#   INFO_ICON          embeds the info tooltip icon. Opt-in: info.png is ~165 KB.
#   TRANSLATION_EXTRA  further files whose text the font subset has to cover, for strings that do
#                      not appear in translations.json
#   IL2CPP_API         RUNTIME il2cpp only: also compile common/include/il2cpp/il2cpp.cpp. Leave off
#                      when the game's IL2CPP runtime is protected and resolved by memory reads.
#   SDK_SOURCES        RUNTIME ue only, and required there: the generated Dumper-7 *_functions.cpp
#                      to compile, which differ per game.
#
# ------------------------------------------------------------------------------------------------
# RUNTIME
#
# It selects the payload built and embedded as RCDATA for the trainer to inject at run time, and
# (for cdp only) what the executable links:
#
#   none     TrainerBase only; no payload, nothing extra linked
#   mono     Mono.cs -> Mono.dll, embedded with MonoBridge.dll (a build-order dep, never linked)
#   il2cpp   IL2CPP.cpp -> IL2CPP.dll against MinHook; adds il2cpp.cpp under IL2CPP_API
#   ue       UE.cpp plus the game's Dumper-7 SDK -> UE.dll against MinHook
#   cdp      no payload - drives the game over the DevTools protocol; links winhttp, ole32, comdlg32
#
# The name is derived, not declared: the folder name is already the product name, the executable
# name and the window title's translations.json key, so deriving keeps those in step. A folder
# cannot hold a colon, so "_" in the name stands for ": " and is restored for the title.
#
# Trainers predating this function set GAME_NAME / TRAINER_NAME / TRAINER_ARCH themselves and build
# through their own CMakeLists.txt; nothing here touches them.

function(add_trainer)
    cmake_parse_arguments(ARG
        "INFO_ICON;IL2CPP_API"
        "ARCH;RUNTIME;GAME_VERSION;TRAINER_VERSION;COLUMN_ALIGN;ROW_GAP"
        "WINDOW;COLUMN_GAPS;SDK_SOURCES;TRANSLATION_EXTRA"
        ${ARGN})

    set(TRAINER_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    get_filename_component(TRAINER_NAME "${TRAINER_DIR}" NAME)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" GAME_NAME "${TRAINER_NAME}")

    # A colon is illegal in a Windows path, so a subtitled game is stored with "_" standing in for
    # ": ". The folder and the executable keep the underscore; the window title and its
    # translations.json key use the real name.
    string(REPLACE "_" ": " TRAINER_DISPLAY_NAME "${TRAINER_NAME}")

    # Validate the manifest.
    # DEFINED, not truthiness: CMake reads "0" as false, which would reject a gap or version of "0".
    foreach(required ARCH RUNTIME GAME_VERSION TRAINER_VERSION WINDOW COLUMN_GAPS COLUMN_ALIGN ROW_GAP)
        if(NOT DEFINED ARG_${required} OR ARG_${required} STREQUAL "")
            message(FATAL_ERROR "add_trainer(): ${required} is required for ${TRAINER_NAME}")
        endif()
    endforeach()

    if(NOT ARG_ARCH MATCHES "^(x64|x86)$")
        message(FATAL_ERROR "add_trainer(): unsupported ARCH '${ARG_ARCH}' (expected: x64, x86)")
    endif()
    if(NOT ARG_RUNTIME MATCHES "^(none|mono|il2cpp|ue|cdp)$")
        message(FATAL_ERROR
            "add_trainer(): unsupported RUNTIME '${ARG_RUNTIME}' (expected: none, mono, il2cpp, ue, cdp)")
    endif()

    list(LENGTH ARG_WINDOW WINDOW_PARTS)
    if(NOT WINDOW_PARTS EQUAL 2)
        message(FATAL_ERROR "add_trainer(): WINDOW takes a width and a height, got '${ARG_WINDOW}'")
    endif()
    list(GET ARG_WINDOW 0 WINDOW_W)
    list(GET ARG_WINDOW 1 WINDOW_H)

    # Mapped here so an unknown alignment fails at configure time rather than in the compiler.
    if(ARG_COLUMN_ALIGN STREQUAL "top")
        set(COLUMN_ALIGN_TOKEN "ColumnAlign::Top")
    elseif(ARG_COLUMN_ALIGN STREQUAL "center")
        set(COLUMN_ALIGN_TOKEN "ColumnAlign::Center")
    else()
        message(FATAL_ERROR
            "add_trainer(): unknown COLUMN_ALIGN '${ARG_COLUMN_ALIGN}' (expected: top, center)")
    endif()

    # A comma-separated initialiser, so the array length is the column count and nothing is parsed.
    string(REPLACE ";" "," COLUMN_GAPS_INIT "${ARG_COLUMN_GAPS}")

    # An option that means nothing for this runtime is an error, not a silent no-op.
    if(ARG_IL2CPP_API AND NOT ARG_RUNTIME STREQUAL "il2cpp")
        message(FATAL_ERROR "add_trainer(): IL2CPP_API only applies to RUNTIME il2cpp")
    endif()
    if(ARG_SDK_SOURCES AND NOT ARG_RUNTIME STREQUAL "ue")
        message(FATAL_ERROR "add_trainer(): SDK_SOURCES only applies to RUNTIME ue")
    endif()
    if(ARG_RUNTIME STREQUAL "ue" AND NOT ARG_SDK_SOURCES)
        message(FATAL_ERROR "add_trainer(): RUNTIME ue needs SDK_SOURCES (the Dumper-7 *_functions.cpp)")
    endif()

    set(TRAINER_DEPS "")
    set(TRAINER_LIBS "")
    set(RC_DEPENDS "")
    set(RC_EXTRA "")

    set(MINHOOK_LIB "${CMAKE_SOURCE_DIR}/common/libs/${ARG_ARCH}/libMinHook.${ARG_ARCH}.lib")

    # Build output the trainer embeds but does not keep in source. The generated resources.rc
    # references it by absolute path, so the trainer's folder stays clean.
    set(STAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/staged")

    # Font subset, cut down to the glyphs this trainer's text uses.
    set(FONT_OUTPUT "${STAGE_DIR}/NotoSansSC-Subset.ttf")
    set(FONT_INPUTS "${TRAINER_DIR}/translations.json" ${ARG_TRANSLATION_EXTRA})
    add_custom_command(
        OUTPUT "${FONT_OUTPUT}"
        COMMAND "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe"
                "${CMAKE_SOURCE_DIR}/scripts/font_processor.py"
                ${FONT_INPUTS}
                --font "${CMAKE_SOURCE_DIR}/scripts/NotoSansSC-Regular.ttf"
                --output "${FONT_OUTPUT}"
        DEPENDS "${CMAKE_SOURCE_DIR}/scripts/font_processor.py"
                "${CMAKE_SOURCE_DIR}/scripts/NotoSansSC-Regular.ttf"
                ${FONT_INPUTS}
        COMMENT "Subsetting NotoSansSC-Regular.ttf for ${TRAINER_NAME}"
        VERBATIM
    )
    add_custom_target(${GAME_NAME}_SubsetFont ALL DEPENDS "${FONT_OUTPUT}")
    list(APPEND TRAINER_DEPS ${GAME_NAME}_SubsetFont)
    list(APPEND RC_DEPENDS "${FONT_OUTPUT}")

    # Payload injected into the game.
    if(ARG_RUNTIME STREQUAL "mono")
        foreach(needed Mono.cs Mono.csproj)
            if(NOT EXISTS "${TRAINER_DIR}/${needed}")
                message(FATAL_ERROR "add_trainer(): RUNTIME mono needs ${needed} in ${TRAINER_NAME}")
            endif()
        endforeach()

        set(MONO_DLL "${STAGE_DIR}/Mono.dll")
        set(MONO_TEMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/Mono")

        # BaseOutputPath is deliberately unset: it conflicts with -o and silently produces no
        # assembly. Mono.csproj also disables the SDK's default compile glob, so a stray obj/ left by
        # the IDE cannot be swept in (CS0579 duplicate assembly attributes).
        add_custom_command(
            OUTPUT "${MONO_TEMP_DIR}/bin/Mono.dll"
            COMMAND "dotnet" build "${TRAINER_DIR}/Mono.csproj" -c Release -o "${MONO_TEMP_DIR}/bin"
                    "-p:BaseIntermediateOutputPath=${MONO_TEMP_DIR}/obj/"
                    "-p:MSBuildProjectExtensionsPath=${MONO_TEMP_DIR}/obj/"
            DEPENDS "${TRAINER_DIR}/Mono.cs" "${TRAINER_DIR}/Mono.csproj"
            COMMENT "Compiling Mono.cs for ${TRAINER_NAME}"
            VERBATIM
        )
        add_custom_command(
            OUTPUT "${MONO_DLL}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MONO_TEMP_DIR}/bin/Mono.dll" "${MONO_DLL}"
            DEPENDS "${MONO_TEMP_DIR}/bin/Mono.dll"
            COMMENT "Staging Mono.dll for ${TRAINER_NAME}"
        )
        add_custom_target(${GAME_NAME}_Mono ALL DEPENDS "${MONO_DLL}")

        # MonoBridge is built first so its DLL exists to embed; it is not linked.
        list(APPEND TRAINER_DEPS ${GAME_NAME}_Mono MonoBridge)
        list(APPEND RC_DEPENDS "${MONO_DLL}")
        string(APPEND RC_EXTRA
            "MONOBRIDGE_DLL RCDATA \"${CMAKE_SOURCE_DIR}/common/libs/${ARG_ARCH}/MonoBridge.dll\"\n"
            "MONO_DLL RCDATA \"${MONO_DLL}\"\n")

    elseif(ARG_RUNTIME MATCHES "^(il2cpp|ue)$")
        # Both build one native DLL against MinHook; only the sources and the name differ.
        if(ARG_RUNTIME STREQUAL "il2cpp")
            set(PAYLOAD_NAME "IL2CPP")
            set(PAYLOAD_SOURCES "${TRAINER_DIR}/IL2CPP.cpp")
            if(ARG_IL2CPP_API)
                list(APPEND PAYLOAD_SOURCES "${CMAKE_SOURCE_DIR}/common/include/il2cpp/il2cpp.cpp")
            endif()
        else()
            set(PAYLOAD_NAME "UE")
            set(PAYLOAD_SOURCES "${TRAINER_DIR}/UE.cpp" ${ARG_SDK_SOURCES})
        endif()

        if(NOT EXISTS "${TRAINER_DIR}/${PAYLOAD_NAME}.cpp")
            message(FATAL_ERROR
                "add_trainer(): RUNTIME ${ARG_RUNTIME} needs ${PAYLOAD_NAME}.cpp in ${TRAINER_NAME}")
        endif()

        set(PAYLOAD_TARGET "${GAME_NAME}_${PAYLOAD_NAME}")
        set(PAYLOAD_DLL "${STAGE_DIR}/${PAYLOAD_NAME}.dll")

        add_library(${PAYLOAD_TARGET} SHARED ${PAYLOAD_SOURCES})
        target_link_libraries(${PAYLOAD_TARGET} PRIVATE Common "${MINHOOK_LIB}")
        set_target_properties(${PAYLOAD_TARGET} PROPERTIES
            OUTPUT_NAME "${PAYLOAD_NAME}"
            WINDOWS_EXPORT_ALL_SYMBOLS OFF
        )

        if(ARG_RUNTIME STREQUAL "ue")
            set(UE_SDK_DIR "${TRAINER_DIR}/CppSDK")
            if(NOT EXISTS "${UE_SDK_DIR}/SDK/Basic.hpp")
                message(FATAL_ERROR "add_trainer(): Dumper-7 SDK not found at ${UE_SDK_DIR}")
            endif()
            target_include_directories(${PAYLOAD_TARGET} PRIVATE "${UE_SDK_DIR}")
            target_compile_definitions(${PAYLOAD_TARGET} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
            if(MSVC)
                # Dumper-7 SDK translation units are large enough to need /bigobj.
                target_compile_options(${PAYLOAD_TARGET} PRIVATE /bigobj /MP4 /W0)
            endif()
        endif()

        add_custom_command(TARGET ${PAYLOAD_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${PAYLOAD_TARGET}>" "${PAYLOAD_DLL}"
            COMMENT "Staging ${PAYLOAD_NAME}.dll for ${TRAINER_NAME}"
            VERBATIM
        )

        # A Dumper-7 dump is far larger than any trainer compiles, so prune_cpp_sdk.py deletes the
        # files the compiler never opened, reading MSBuild's CL.read.*.tlog logs to know which.
        # Conditional on the generator, not the compiler: MSVC under Ninja writes no tlogs and has no
        # <target>.dir/<Config> layout. The script itself keeps this to Release, since a Debug build
        # reads a different set of files.
        if(ARG_RUNTIME STREQUAL "ue" AND CMAKE_GENERATOR MATCHES "Visual Studio")
            add_custom_command(TARGET ${PAYLOAD_TARGET} POST_BUILD
                COMMAND "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe"
                    "${CMAKE_SOURCE_DIR}/scripts/prune_cpp_sdk.py"
                    "${UE_SDK_DIR}"
                    "${CMAKE_CURRENT_BINARY_DIR}/${PAYLOAD_TARGET}.dir/$<CONFIG>"
                    --configuration "$<CONFIG>"
                COMMENT "Pruning unused Dumper-7 CppSDK files for ${TRAINER_NAME}"
                VERBATIM
            )
        endif()

        list(APPEND TRAINER_DEPS ${PAYLOAD_TARGET})
        list(APPEND RC_DEPENDS "${PAYLOAD_DLL}")
        string(APPEND RC_EXTRA "${PAYLOAD_NAME}_DLL RCDATA \"${PAYLOAD_DLL}\"\n")

    elseif(ARG_RUNTIME STREQUAL "cdp")
        # No payload - the trainer drives the game over a WebSocket. winhttp opens it, comdlg32 is
        # the browse dialog, ole32 the shell calls around it. ws2_32 is already in Common.
        list(APPEND TRAINER_LIBS winhttp ole32 comdlg32)
    endif()

    # Resource script: absolute paths, since it lives in the build tree, not beside what it names.
    set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/resources.rc")
    set(RC_CONTENT "APP_ICON ICON \"${CMAKE_SOURCE_DIR}/common/assets/logo.ico\"\n")
    string(APPEND RC_CONTENT "LOGO_IMG RCDATA \"${TRAINER_DIR}/logo.jpg\"\n")
    string(APPEND RC_CONTENT "TRANSLATION_JSON RCDATA \"${TRAINER_DIR}/translations.json\"\n")
    string(APPEND RC_CONTENT "FONT_TTF RCDATA \"${FONT_OUTPUT}\"\n")
    if(ARG_INFO_ICON)
        string(APPEND RC_CONTENT "INFO_IMG RCDATA \"${CMAKE_SOURCE_DIR}/common/assets/info.png\"\n")
    endif()
    string(APPEND RC_CONTENT "${RC_EXTRA}")
    file(WRITE "${RC_FILE}" "${RC_CONTENT}")

    # Executable.
    add_executable(${GAME_NAME} "${TRAINER_DIR}/main.cpp" "${RC_FILE}")
    target_link_libraries(${GAME_NAME} PRIVATE Common ${TRAINER_LIBS})
    add_dependencies(${GAME_NAME} ${TRAINER_DEPS})
    set_target_properties(${GAME_NAME} PROPERTIES
        WIN32_EXECUTABLE $<$<NOT:$<CONFIG:Debug>>:ON>
        OUTPUT_NAME "${TRAINER_NAME}"
    )

    # What TrainerApp.h builds its TrainerAppInfo from, so main.cpp declares none of it.
    target_compile_definitions(${GAME_NAME} PRIVATE
        TRAINER_APP_NAME="${TRAINER_DISPLAY_NAME}"
        TRAINER_APP_GAME_VERSION="${ARG_GAME_VERSION}"
        TRAINER_APP_TRAINER_VERSION="${ARG_TRAINER_VERSION}"
        TRAINER_APP_WIDTH=${WINDOW_W}
        TRAINER_APP_HEIGHT=${WINDOW_H}
        TRAINER_APP_COLUMN_GAPS=${COLUMN_GAPS_INIT}
        TRAINER_APP_COLUMN_ALIGN=${COLUMN_ALIGN_TOKEN}
        TRAINER_APP_ROW_GAP=${ARG_ROW_GAP}
    )
    set_source_files_properties("${RC_FILE}" PROPERTIES
        LANGUAGE RC
        OBJECT_DEPENDS "${RC_DEPENDS}"
    )

    add_custom_command(TARGET ${GAME_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${GAME_NAME}>"
            "${CMAKE_BINARY_DIR}/bin/${TRAINER_NAME}/${TRAINER_NAME}.exe"
        COMMAND "mt.exe"
            -manifest "${CMAKE_SOURCE_DIR}/common/assets/elevate.xml"
            "-outputresource:${CMAKE_BINARY_DIR}/bin/${TRAINER_NAME}/${TRAINER_NAME}.exe;1"
        COMMENT "Moving ${TRAINER_NAME}.exe to ${CMAKE_BINARY_DIR}/bin/${TRAINER_NAME} and embedding manifest"
    )
endfunction()
