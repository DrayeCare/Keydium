cmake_minimum_required(VERSION 3.22)
project(MidiPiano VERSION 1.0.0 LANGUAGES C CXX)

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

juce_add_gui_app(MidiPiano
    PRODUCT_NAME "Midi Piano KeyMapper"
)

juce_generate_juce_header(MidiPiano)

target_sources(MidiPiano PRIVATE
    src/main.cpp
    src/gui/MainComponent.cpp
    src/gui/PianoKeyboard.cpp
)

target_include_directories(MidiPiano PRIVATE include)

target_compile_definitions(MidiPiano PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="Midi Piano KeyMapper"
    JUCE_APPLICATION_VERSION_STRING="1.0.0"
)

target_link_libraries(MidiPiano PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)