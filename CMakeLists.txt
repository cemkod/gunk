cmake_minimum_required(VERSION 3.22)
project(BassSynth VERSION 1.0.0)

find_package(JUCE CONFIG REQUIRED)

juce_add_plugin(BassSynth
    COMPANY_NAME "JimQode"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Jqod
    PLUGIN_CODE Bsyn
    FORMATS VST3 Standalone
    PRODUCT_NAME "Bass Synth"
)

juce_generate_juce_header(BassSynth)

target_sources(BassSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Oscillator.cpp
)

target_compile_definitions(BassSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_FLAC=0
    JUCE_USE_OGGVORBIS=0
    $<$<CONFIG:RelWithDebInfo>:ENABLE_DEBUG_LOG>
)

target_link_libraries(BassSynth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        $<$<CONFIG:Release>:juce::juce_recommended_lto_flags>
        juce::juce_recommended_warning_flags
)
