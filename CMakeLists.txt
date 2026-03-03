cmake_minimum_required(VERSION 3.22)
project(JQGunk VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)

if(CMAKE_CROSSCOMPILING)
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
    find_package(JUCE CONFIG QUIET)
    if(NOT JUCE_FOUND)
        FetchContent_MakeAvailable(JUCE)
    endif()
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
    Source/CombinedOscSectionComponent.cpp
    Source/FilterSectionComponent.cpp
    Source/FilterDisplayComponent.cpp
    Source/PresetManager.cpp
    Source/ModMatrix.h
    Source/ModMatrixComponent.h
    Source/ModMatrixComponent.cpp
)

if(UNIX AND NOT APPLE)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/Presets/Factory/"
            DESTINATION "$ENV{HOME}/.config/JQGunk/Factory Presets"
            EXCLUDE_FROM_ALL)

    # System-wide install rules used by CPack
    install(DIRECTORY
        "${CMAKE_BINARY_DIR}/JQGunk_artefacts/Release/VST3/JQ Gunk.vst3"
        DESTINATION "lib/vst3"
        COMPONENT vst3
    )
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/Presets/Factory/"
        DESTINATION "share/JQGunk/Factory Presets"
        COMPONENT vst3
    )

    # CPack
    set(CPACK_PACKAGE_NAME "jq-gunk")
    set(CPACK_PACKAGE_VENDOR "JimQode")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "JQ Gunk bass synth VST3 plugin")
    set(CPACK_PACKAGE_CONTACT "contact@jimqode.com")
    set(CPACK_COMPONENTS_ALL vst3)
    set(CPACK_STRIP_FILES TRUE)
    # Only install our vst3 component — exclude JUCE's own install rules
    set(CPACK_INSTALL_CMAKE_PROJECTS
        "${CMAKE_BINARY_DIR}" "${PROJECT_NAME}" "vst3" "/"
    )

    set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libasound2t64")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
    set(CPACK_RPM_PACKAGE_LICENSE "Proprietary")
    set(CPACK_RPM_PACKAGE_REQUIRES "alsa-lib")
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

    include(CPack)
endif()

add_custom_target(install-presets
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$ENV{HOME}/.config/JQGunk/Factory Presets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Presets/Factory"
        "$ENV{HOME}/.config/JQGunk/Factory Presets"
    COMMENT "Installing factory presets to ~/.config/JQGunk/Factory Presets"
)

add_custom_target(install-wavetables
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$ENV{HOME}/.config/JQGunk/Wavetables"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Wavetables"
        "$ENV{HOME}/.config/JQGunk/Wavetables"
    COMMENT "Installing factory wavetables to ~/.config/JQGunk/Wavetables"
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

# Statically link MinGW runtimes on Windows cross-compile so the VST3/exe
# has no libgcc_s_seh-1.dll / libstdc++-6.dll / libwinpthread-1.dll dependencies.
# libwinpthread cannot be fully statically absorbed (libstdc++ depends on it at
# runtime), so it is bundled inside the VST3 package instead — see bundle-windows.
if(CMAKE_CROSSCOMPILING AND WIN32)
    set(_mingw_static_flags
        -static-libgcc
        -static-libstdc++
        /usr/x86_64-w64-mingw32/lib/libwinpthread.a
    )
    target_link_options(JQGunk_VST3 PRIVATE ${_mingw_static_flags})
    target_link_options(JQGunk_Standalone PRIVATE ${_mingw_static_flags})

    # Paths
    set(_vst3_src "${CMAKE_BINARY_DIR}/JQGunk_artefacts/Release/VST3/JQ Gunk.vst3")
    set(_winpthread_dll "/usr/x86_64-w64-mingw32/bin/libwinpthread-1.dll")
    set(_bundle_dir "${CMAKE_BINARY_DIR}/bundle-windows")
    set(_install_txt "${CMAKE_SOURCE_DIR}/packaging/windows-install.txt")
    set(_zip_out "${CMAKE_BINARY_DIR}/JQGunk-windows-vst3.zip")

    add_custom_target(bundle-windows
        DEPENDS JQGunk_VST3
        # 1. Recreate staging dir
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${_bundle_dir}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${_bundle_dir}/JQ Gunk.vst3/Contents/x86_64-win"
            "${_bundle_dir}/JQ Gunk.vst3/Contents/Resources"
        # 2. Copy VST3 contents
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${_vst3_src}"
            "${_bundle_dir}/JQ Gunk.vst3"
        # 3. Bundle libwinpthread-1.dll next to the plugin DLL
        COMMAND ${CMAKE_COMMAND} -E copy
            "${_winpthread_dll}"
            "${_bundle_dir}/JQ Gunk.vst3/Contents/x86_64-win/libwinpthread-1.dll"
        # 4. Copy install instructions
        COMMAND ${CMAKE_COMMAND} -E copy
            "${_install_txt}"
            "${_bundle_dir}/INSTALL.txt"
        # 5. Zip
        COMMAND ${CMAKE_COMMAND} -E rm -f "${_zip_out}"
        COMMAND ${CMAKE_COMMAND} -E chdir "${_bundle_dir}"
            ${CMAKE_COMMAND} -E tar cf "${_zip_out}" "--format=zip" "."
        COMMENT "Building Windows VST3 bundle: ${_zip_out}"
    )
endif()
