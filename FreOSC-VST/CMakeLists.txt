cmake_minimum_required(VERSION 3.22)

project(FreOSC-VST VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as subdirectory
# Check if JUCE_DIR is set via environment variable or command line
if(DEFINED ENV{JUCE_DIR})
    set(JUCE_DIR "$ENV{JUCE_DIR}")
endif()

# Try to find JUCE_DIR or set a default
if(NOT JUCE_DIR OR NOT EXISTS "${JUCE_DIR}")
    # Try some common locations
    set(POSSIBLE_JUCE_DIRS 
        "C:/Users/DomenicSimone/Personal/freosc/JUCE"
        "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE"
        "C:/JUCE"
        "C:/Program Files/JUCE")
    
    foreach(DIR ${POSSIBLE_JUCE_DIRS})
        if(EXISTS "${DIR}/CMakeLists.txt")
            set(JUCE_DIR "${DIR}")
            break()
        endif()
    endforeach()
    
    if(NOT JUCE_DIR OR NOT EXISTS "${JUCE_DIR}")
        message(FATAL_ERROR "Could not find JUCE. Please set JUCE_DIR to your JUCE installation directory")
    endif()
endif()

message(STATUS "Using JUCE from: ${JUCE_DIR}")
add_subdirectory("${JUCE_DIR}" JUCE)

# Define plugin formats to build
set(FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND FORMATS AU)
endif()

# Create the plugin target
juce_add_plugin(FreOSC-VST
    COMPANY_NAME "FreOSC"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Freo
    PLUGIN_CODE Fosc
    FORMATS ${FORMATS}
    PRODUCT_NAME "FreOSC")

# Set source files
set(SOURCE_FILES
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    
    # DSP Classes
    Source/DSP/FreOscVoice.cpp
    Source/DSP/FreOscVoice.h
    Source/DSP/FreOscSound.cpp
    Source/DSP/FreOscSound.h
    Source/DSP/FreOscOscillator.cpp
    Source/DSP/FreOscOscillator.h
    Source/DSP/FreOscFilter.cpp
    Source/DSP/FreOscFilter.h
    Source/DSP/FreOscLFO.cpp
    Source/DSP/FreOscLFO.h
    Source/DSP/FreOscPlateReverb.cpp
    Source/DSP/FreOscPlateReverb.h
    Source/DSP/FreOscTapeDelay.cpp
    Source/DSP/FreOscTapeDelay.h
    Source/DSP/FreOscWavefolder.cpp
    Source/DSP/FreOscWavefolder.h
    Source/DSP/FreOscNoiseGenerator.cpp
    Source/DSP/FreOscNoiseGenerator.h
    Source/DSP/FreOscEnvelope.cpp
    Source/DSP/FreOscEnvelope.h
    Source/DSP/FreOscCompressor.cpp
    Source/DSP/FreOscCompressor.h
    Source/DSP/FreOscLimiter.cpp
    Source/DSP/FreOscLimiter.h
    
    # Parameters
    Source/Parameters/FreOscParameters.cpp
    Source/Parameters/FreOscParameters.h
    
    # Presets
    Source/Presets/JsonPresetManager.cpp
    Source/Presets/JsonPresetManager.h
)

# Add source files to plugin
target_sources(FreOSC-VST PRIVATE ${SOURCE_FILES})

# Set compile definitions
target_compile_definitions(FreOSC-VST
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

# Link JUCE libraries
target_link_libraries(FreOSC-VST
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Set include directories
target_include_directories(FreOSC-VST
    PRIVATE
        Source
        Source/DSP
        Source/Parameters
        Source/Presets)

# Platform-specific settings
if(APPLE)
    set_target_properties(FreOSC-VST PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
elseif(WIN32)
    set_target_properties(FreOSC-VST PROPERTIES
        WIN32_EXECUTABLE TRUE)
endif()

# Copy resources (presets, etc.) - commented out since Resources directory doesn't exist
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Presets
#      DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Resources/)

# Installation
install(TARGETS FreOSC-VST
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Package configuration
set(CPACK_PACKAGE_NAME "FreOSC-VST")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "FreOSC Software Synthesizer VST Plugin")
set(CPACK_PACKAGE_VENDOR "FreOSC")

include(CPack)