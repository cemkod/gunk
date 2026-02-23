cmake_minimum_required(VERSION 3.22)
project(JQGunk VERSION 1.0.0)

if(CMAKE_CROSSCOMPILING)
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        7.0.12
        GIT_SHALLOW    TRUE
    )

    # juce_vst3_helper dlopen()s the built DLL to extract class IDs, which fails
    # when the DLL is a Windows PE binary on a Linux host. Register a stub before
    # FetchContent_MakeAvailable so JUCE's _juce_add_vst3_manifest_helper_target()
    # sees the target already exists and skips building its own. The stub copies
    # moduleinfo.json from the native Linux build (CIDs are platform-independent).
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vst3_manifest_wrapper.sh.in"
        "${CMAKE_CURRENT_BINARY_DIR}/juce_vst3_helper"
        @ONLY
    )
    file(CHMOD "${CMAKE_CURRENT_BINARY_DIR}/juce_vst3_helper"
         PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                     GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    add_executable(juce_vst3_helper IMPORTED GLOBAL)
    set_target_properties(juce_vst3_helper PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/juce_vst3_helper")

    FetchContent_MakeAvailable(JUCE)
else()
    find_package(JUCE CONFIG REQUIRED)
endif()

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
    Source/FilterDisplayComponent.cpp
    Source/PresetManager.cpp
)

if(UNIX AND NOT APPLE)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/Presets/Factory/"
            DESTINATION "$ENV{HOME}/.config/JQGunk/Factory Presets")
endif()

add_custom_target(install-presets
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$ENV{HOME}/.config/JQGunk/Factory Presets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Presets/Factory"
        "$ENV{HOME}/.config/JQGunk/Factory Presets"
    COMMENT "Installing factory presets to ~/.config/JQGunk/Factory Presets"
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
