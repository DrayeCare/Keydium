
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

juce_add_gui_app(Keydium
    PRODUCT_NAME "Keydium"
)

juce_generate_juce_header(Keydium)

target_sources(Keydium PRIVATE
    src/main.cpp
    src/gui/MainComponent.cpp
    src/gui/PianoKeyboard.cpp
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
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
