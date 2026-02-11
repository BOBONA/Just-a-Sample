cmake_minimum_required(VERSION 3.22)

# High-level configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(APP_VERSION 1.03)
project(JUST_A_SAMPLE VERSION ${APP_VERSION})

include(FetchContent)

# Dependency versions
set(JUCE_VERSION 8.0.12)
set(BUNGEE_INSTALL_VERSION v2.4.0)
set(MELATONIN_BLUR_VERSION origin/main)
set(MELATONIN_INSPECTOR_VERSION origin/main)
set(LEAF_VERSION 8d86c4e96ac48740da34f24c9f995bc3c4b3b2a0)

# Bungee build options
set(BUNGEE_BUILD_SHARED_LIBRARY OFF CACHE INTERNAL "")
set(BUNGEE_BUILD_EXAMPLES OFF CACHE INTERNAL "")

set(STRETCHER_BUNGEE_MAX_OCTAVES 4)

# Fetch dependencies
FetchContent_Declare(
        JUCE
        GIT_REPOSITORY  https://github.com/juce-framework/JUCE.git
        GIT_TAG         ${JUCE_VERSION}
        SOURCE_DIR      ${FETCHCONTENT_BASE_DIR}/JUCE
)

FetchContent_Declare(
        bungee
        GIT_REPOSITORY  https://github.com/bungee-audio-stretch/bungee.git
        GIT_TAG         ${BUNGEE_INSTALL_VERSION}
        SOURCE_DIR      ${FETCHCONTENT_BASE_DIR}/bungee
        # We patch in our own maxPitchOctaves constant
        PATCH_COMMAND git apply -p0 --reverse --check "${CMAKE_CURRENT_SOURCE_DIR}/patches/bungee_octaves.patch"
                || git apply -p0 --ignore-space-change --ignore-whitespace "${CMAKE_CURRENT_SOURCE_DIR}/patches/bungee_octaves.patch")

FetchContent_Declare(
        melatonin_blur
        GIT_REPOSITORY  https://github.com/sudara/melatonin_blur.git
        GIT_TAG         ${MELATONIN_BLUR_VERSION}
        SOURCE_DIR      ${FETCHCONTENT_BASE_DIR}/melatonin_blur
)

FetchContent_Declare(
        melatonin_inspector
        GIT_REPOSITORY  https://github.com/sudara/melatonin_inspector.git
        GIT_TAG         ${MELATONIN_INSPECTOR_VERSION}
        SOURCE_DIR      ${FETCHCONTENT_BASE_DIR}/melatonin_inspector
)

FetchContent_Declare(
        LEAF
        GIT_REPOSITORY  https://github.com/spiricom/LEAF.git
        GIT_TAG         ${LEAF_VERSION}
        SOURCE_DIR      ${FETCHCONTENT_BASE_DIR}/LEAF
)

FetchContent_MakeAvailable(JUCE)
FetchContent_MakeAvailable(bungee)
FetchContent_MakeAvailable(melatonin_blur)
FetchContent_MakeAvailable(melatonin_inspector)
FetchContent_MakeAvailable(LEAF)

# Add leaf (melatonin modules add themselves)
juce_add_module(${FETCHCONTENT_BASE_DIR}/LEAF/leaf)

juce_add_plugin(JustASample
        PRODUCT_NAME                JustASample
        DESCRIPTION                 "Just a Sample is a modern, open-source audio sampler"
        ICON_BIG                    Assets/Icons/IconLogo.svg
        COMPANY_NAME                "Binyamin Friedman"
        COMPANY_COPYRIGHT           "Copyright (c) 2026 Binyamin Friedman"
        COMPANY_WEBSITE             "https://github.com/BOBONA/Just-a-Sample/"
        BUNDLE_ID                   com.BinyaminFriedman.JustASample
        PLUGIN_MANUFACTURER_CODE    Manu
        PLUGIN_CODE                 Bpf2
        FORMATS                     AU VST3
        VST3_CATEGORIES             Sampler
        IS_SYNTH                    TRUE
        NEEDS_MIDI_INPUT            TRUE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

juce_generate_juce_header(JustASample)

target_sources(JustASample
        PRIVATE
        Source/CustomLookAndFeel.cpp
        Source/CustomLookAndFeel.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginParameters.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/Components/Buttons.h
        Source/Components/FxChain.cpp
        Source/Components/FxChain.h
        Source/Components/FxDragTarget.h
        Source/Components/FxModule.cpp
        Source/Components/FxModule.h
        Source/Components/InputDeviceSelector.cpp
        Source/Components/InputDeviceSelector.h
        Source/Components/Prompt.h
        Source/Components/RangeSelector.h
        Source/Components/SampleEditor.cpp
        Source/Components/SampleEditor.h
        Source/Components/SampleNavigator.cpp
        Source/Components/SampleNavigator.h
        Source/Components/Displays/ChorusVisualizer.cpp
        Source/Components/Displays/ChorusVisualizer.h
        Source/Components/Displays/DistortionVisualizer.cpp
        Source/Components/Displays/DistortionVisualizer.h
        Source/Components/Displays/FilterResponse.cpp
        Source/Components/Displays/FilterResponse.h
        Source/Components/Displays/ReverbResponse.cpp
        Source/Components/Displays/ReverbResponse.h
        Source/Components/Displays/SamplePainter.cpp
        Source/Components/Displays/SamplePainter.h
        Source/Sampler/CustomSamplerVoice.cpp
        Source/Sampler/CustomSamplerVoice.h
        Source/Sampler/CustomSynthesizer.h
        Source/Sampler/SamplerParameters.cpp
        Source/Sampler/SamplerParameters.h
        Source/Sampler/Stretcher.h
        Source/Sampler/Effects/BandEQ.h
        Source/Sampler/Effects/Chorus.h
        Source/Sampler/Effects/Distortion.h
        Source/Sampler/Effects/Effect.h
        Source/Sampler/Effects/Reverb.h
        Source/Sampler/Effects/gin/gin_distortion.h
        Source/Sampler/Effects/gin/gin_simpleverb.cpp
        Source/Sampler/Effects/gin/gin_simpleverb.h
        Source/Utilities/BufferUtils.h
        Source/Utilities/ComponentUtils.h
        Source/Utilities/DeviceRecorder.h
        Source/Utilities/ListenableValue.h
        Source/Utilities/PitchDetector.h
        Source/Utilities/SampleLoader.h
        Source/Utilities/MTS/libMTSClient.cpp
        Source/Utilities/MTS/libMTSClient.h
        Source/Utilities/readerwriterqueue/readerwriterqueue.h
        Source/Utilities/readerwriterqueue/atomicops.h
        Source/Utilities/Reaper/ReaperVST3Extensions.cpp
        Source/Utilities/Reaper/ReaperVST3Extensions.h
        Source/Utilities/Reaper/reaper-plugins/reaper_plugin.h
        Source/Utilities/Reaper/reaper-plugins/reaper_vst3_interfaces.h
        Source/Utilities/Reaper/reaper-plugins/swell-types.h
)

target_compile_definitions(JustASample
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_APP_VERSION=${APP_VERSION}
        DONT_SET_USING_JUCE_NAMESPACE=1
        BUNGEE_MAX_OCTAVES=${STRETCHER_BUNGEE_MAX_OCTAVES}
)

juce_add_binary_data(AudioPluginData
        SOURCES
        Assets/Fonts/InriaSans-Bold.ttf
        Assets/Fonts/InriaSans-Regular.ttf
        Assets/Fonts/Inter-Bold.ttf
        Assets/Fonts/Inter-Regular.ttf
        Assets/Icons/IconAdd.svg
        Assets/Icons/IconDarkMode.svg
        Assets/Icons/IconDetect.svg
        Assets/Icons/IconDrag.svg
        Assets/Icons/IconFit.svg
        Assets/Icons/IconHelp.svg
        Assets/Icons/IconHideFX.svg
        Assets/Icons/IconLightMode.svg
        Assets/Icons/IconLinkEnabled.svg
        Assets/Icons/IconLofi.svg
        Assets/Icons/IconLogo.svg
        Assets/Icons/IconLoop.svg
        Assets/Icons/IconLoopStart.svg
        Assets/Icons/IconMono.svg
        Assets/Icons/IconPin.svg
        Assets/Icons/IconPlay.svg
        Assets/Icons/IconPreFX.svg
        Assets/Icons/IconRecordSettings.svg
        Assets/Icons/IconShowFX.svg
        Assets/Icons/IconSpeed.svg
        Assets/Icons/IconWaveformMode.svg
        Assets/Icons/Logo.svg
)

target_link_libraries(JustASample
        PRIVATE
        AudioPluginData
        juce::juce_audio_utils
        juce::juce_cryptography
        juce::juce_dsp
        juce::juce_opengl
        bungee_library
        melatonin_blur
        $<$<CONFIG:Debug>:melatonin_inspector>  # Only link melatonin_inspector in Debug builds
        leaf
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_include_directories(JustASample
        PRIVATE
        "${bungee_SOURCE_DIR}/bungee"
)

target_compile_definitions(bungee_library PRIVATE BUNGEE_MAX_OCTAVES=${STRETCHER_BUNGEE_MAX_OCTAVES})

# Bungee requires some patching on MSVC
if (MSVC)
    # Add a dummy unistd.h
    target_include_directories(bungee_library PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/fake_headers)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/fake_headers/unistd.h "")

    # Add a header to define remove __attribute__, since MSVC doesn't support it
    set(BUNGEE_COMPAT_H "${CMAKE_CURRENT_BINARY_DIR}/bungee_msvc_compat.h")
    file(WRITE "${BUNGEE_COMPAT_H}" "
#ifndef BUNGEE_MSVC_COMPAT_H
#define BUNGEE_MSVC_COMPAT_H
#define __attribute__(x)
#endif
")

    target_compile_options(bungee_library PRIVATE "SHELL:/FI \"${BUNGEE_COMPAT_H}\"")

    # Use _USE_MATH_DEFINES to get M_PI and other math constants
    target_compile_definitions(pffft PRIVATE _USE_MATH_DEFINES)

    target_compile_options(pffft PRIVATE "/wd9002" "/fp:fast" "/GS-")
endif()