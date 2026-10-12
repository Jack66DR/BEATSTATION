cmake_minimum_required(VERSION 3.22)
project(BEATSTATION VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BEATSTATION_BUILD_TESTS "Build the offline engine test and UI snapshot tools" OFF)
set(BEATSTATION_JUCE_PATH "" CACHE PATH "Use a local JUCE checkout instead of downloading")

if(BEATSTATION_JUCE_PATH)
    add_subdirectory(${BEATSTATION_JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(DS_FORMATS AU VST3)
else()
    set(DS_FORMATS VST3)
endif()

juce_add_plugin(BEATSTATION
    COMPANY_NAME "manners! DSP"
    PLUGIN_MANUFACTURER_CODE Mnrs
    PLUGIN_CODE Btst
    FORMATS ${DS_FORMATS}
    PRODUCT_NAME "BEATSTATION"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    BUNDLE_ID com.mannersdsp.beatstation)

set(DS_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_sources(BEATSTATION PRIVATE ${DS_SOURCES})

target_compile_definitions(BEATSTATION PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(BEATSTATION
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(BEATSTATION_BUILD_TESTS)
    foreach(tool Test Snapshot)
        juce_add_console_app(BEATSTATION_${tool} PRODUCT_NAME "BEATSTATION_${tool}")
        target_sources(BEATSTATION_${tool} PRIVATE Tests/${tool}.cpp ${DS_SOURCES})
        target_include_directories(BEATSTATION_${tool} PRIVATE Source)
        target_compile_definitions(BEATSTATION_${tool} PRIVATE
            JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_STANDALONE_APPLICATION=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
        target_link_libraries(BEATSTATION_${tool} PRIVATE
            juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
    endforeach()
endif()
