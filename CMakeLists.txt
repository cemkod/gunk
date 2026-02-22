cmake_minimum_required(VERSION 3.22)
project(JQGunk VERSION 1.0.0)

find_package(JUCE CONFIG REQUIRED)

juce_add_plugin(JQGunk
    COMPANY_NAME "JimQode"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Jqod
    PLUGIN_CODE JQGk
    FORMATS VST3 Standalone
    PRODUCT_NAME "JQ Gunk"
)

juce_add_binary_data(JQGunkData SOURCES logo.svg)

juce_generate_juce_header(JQGunk)

target_sources(JQGunk PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Oscillator.cpp
    Source/PitchDetector.cpp
    Source/GateSectionComponent.cpp
    Source/OscSectionComponent.cpp
    Source/FilterSectionComponent.cpp
)

target_compile_definitions(JQGunk PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_FLAC=0
    JUCE_USE_OGGVORBIS=0
    $<$<CONFIG:RelWithDebInfo>:ENABLE_DEBUG_LOG>
)

target_link_libraries(JQGunk
    PRIVATE
        JQGunkData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        $<$<CONFIG:Release>:juce::juce_recommended_lto_flags>
        juce::juce_recommended_warning_flags
)
