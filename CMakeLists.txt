
cmake_minimum_required(VERSION 3.22)

project(Keydium VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.9
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

# ─── FluidSynth (official Windows MSVC binaries) ─────────────────────────────
# Path to the extracted FluidSynth release directory
set(FLUIDSYNTH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/libs/fluidsynth/fluidsynth-v2.6.2-win10-x64-cpp11")

# Create an IMPORTED target so we can link naturally
add_library(FluidSynth::FluidSynth SHARED IMPORTED)
set_target_properties(FluidSynth::FluidSynth PROPERTIES
    IMPORTED_LOCATION           "${FLUIDSYNTH_ROOT}/bin/libfluidsynth-3.dll"
    IMPORTED_IMPLIB             "${FLUIDSYNTH_ROOT}/lib/libfluidsynth-3.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${FLUIDSYNTH_ROOT}/include"
)

juce_add_gui_app(Keydium
    PRODUCT_NAME "Keydium"
)

juce_generate_juce_header(Keydium)

target_sources(Keydium PRIVATE
    src/main.cpp
    src/gui/MainComponent.cpp
    src/gui/PianoKeyboard.cpp
    src/core/PianoAudioEngine.cpp
    src/core/NoteTracker.cpp
)

target_include_directories(Keydium PRIVATE include)

target_compile_definitions(Keydium PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="Keydium"
    JUCE_APPLICATION_VERSION_STRING="1.0.0"
)

target_link_libraries(Keydium PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_extra
    FluidSynth::FluidSynth
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# ─── Copy FluidSynth DLLs next to the executable ─────────────────────────────
# This ensures the app can find libfluidsynth-3.dll and its dependencies
# at runtime without requiring a system-wide install.
set(FLUID_BIN_DIR "${FLUIDSYNTH_ROOT}/bin")
set(FLUID_DLLS
    "${FLUID_BIN_DIR}/libfluidsynth-3.dll"
    "${FLUID_BIN_DIR}/SDL3.dll"
    "${FLUID_BIN_DIR}/sndfile.dll"
)

foreach(DLL ${FLUID_DLLS})
    add_custom_command(TARGET Keydium POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL}"
            "$<TARGET_FILE_DIR:Keydium>"
        COMMENT "Copying ${DLL} to output directory"
    )
endforeach()

# Also copy the SoundFont next to the executable for runtime discovery
add_custom_command(TARGET Keydium POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/sounds/GeneralUser-GS.sf2"
        "$<TARGET_FILE_DIR:Keydium>/GeneralUser-GS.sf2"
    COMMENT "Copying GeneralUser-GS.sf2 to output directory"
)

# ─── Unit Test Target ────────────────────────────────────────────────────────
add_executable(PianoAudioEngineTest
    tests/test_audio_engine.cpp
    src/core/PianoAudioEngine.cpp
    src/core/NoteTracker.cpp
)

target_include_directories(PianoAudioEngineTest PRIVATE include)

target_link_libraries(PianoAudioEngineTest PRIVATE
    FluidSynth::FluidSynth
)

foreach(DLL ${FLUID_DLLS})
    add_custom_command(TARGET PianoAudioEngineTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL}"
            "$<TARGET_FILE_DIR:PianoAudioEngineTest>"
        COMMENT "Copying ${DLL} to test output directory"
    )
endforeach()

