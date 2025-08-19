#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

//==============================================================================
/**
    JSON-Based Preset Manager for FreOSC-VST

    Modern preset system using human-readable JSON files with actual parameter values.
    Presets are stored as individual .json files in the Presets/ folder.

    Features:
    - Human-readable JSON format with real values (Hz, dB, etc.)
    - Easy to create/edit presets by hand
    - Automatic parameter conversion and validation
    - Factory presets as separate files
    - User preset support
*/
class JsonPresetManager
{
public:
    //==============================================================================
    JsonPresetManager();
    ~JsonPresetManager();

    //==============================================================================
    // Initialization
    void initialize(const juce::File& presetFolder);
    void scanForPresets();

    //==============================================================================
    // Preset operations
    bool loadPreset(int presetIndex, juce::AudioProcessorValueTreeState& parameters);
    bool loadPreset(const juce::String& presetName, juce::AudioProcessorValueTreeState& parameters);
    bool savePreset(const juce::String& presetName, juce::AudioProcessorValueTreeState& parameters);
    bool saveCurrentAsPreset(const juce::String& name, juce::AudioProcessorValueTreeState& parameters);
    
    // New preset management operations
    bool updatePreset(int presetIndex, juce::AudioProcessorValueTreeState& parameters);
    bool updatePreset(const juce::String& presetName, juce::AudioProcessorValueTreeState& parameters);
    bool deletePreset(int presetIndex);
    bool deletePreset(const juce::String& presetName);
    bool saveUserPreset(const juce::String& name, const juce::String& description, juce::AudioProcessorValueTreeState& parameters);
    bool presetExists(const juce::String& presetName) const;

    //==============================================================================
    // Preset information
    juce::StringArray getPresetNames() const;
    int getNumPresets() const { return static_cast<int>(presets.size()); }
    juce::String getPresetName(int index) const;
    int getCurrentPresetIndex() const { return currentPresetIndex; }

    // Get preset metadata
    juce::String getPresetDescription(int index) const;
    bool isFactoryPreset(int index) const;
    
    // Current preset state tracking
    juce::String getCurrentPresetName() const;
    void setCurrentPreset(int index);
    void setCurrentPreset(const juce::String& name);
    void clearCurrentPreset(); // Sets to "Default" state

private:
    //==============================================================================
    struct PresetInfo
    {
        juce::String name;
        juce::String description;
        juce::File file;
        bool isFactory;
        juce::var jsonData;

        PresetInfo(const juce::String& n, const juce::File& f, bool factory = false)
            : name(n), file(f), isFactory(factory) {}
    };

    //==============================================================================
    std::vector<PresetInfo> presets;
    juce::File presetFolder;
    int currentPresetIndex = -1;

    //==============================================================================
    // JSON processing
    bool loadPresetFromFile(const juce::File& file, juce::AudioProcessorValueTreeState& parameters);
    bool savePresetToFile(const juce::File& file, const juce::String& name, const juce::String& description, juce::AudioProcessorValueTreeState& parameters);

    // Parameter conversion - using simple normalized format only
    juce::var createSimplePresetJson(const juce::String& name, const juce::String& description, juce::AudioProcessorValueTreeState& parameters);
    bool applyPresetJson(const juce::var& presetData, juce::AudioProcessorValueTreeState& parameters);
    bool applySimplePresetFormat(const juce::var& parametersData, juce::AudioProcessorValueTreeState& parameters);

    // No conversion helpers needed - using normalized values directly

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JsonPresetManager)
};