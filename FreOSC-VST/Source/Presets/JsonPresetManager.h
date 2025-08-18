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

    //==============================================================================
    // Preset information
    juce::StringArray getPresetNames() const;
    int getNumPresets() const { return static_cast<int>(presets.size()); }
    juce::String getPresetName(int index) const;
    int getCurrentPresetIndex() const { return currentPresetIndex; }

    // Get preset metadata
    juce::String getPresetDescription(int index) const;
    bool isFactoryPreset(int index) const;

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

    // Parameter conversion
    juce::var createPresetJson(const juce::String& name, const juce::String& description, juce::AudioProcessorValueTreeState& parameters);
    bool applyPresetJson(const juce::var& presetData, juce::AudioProcessorValueTreeState& parameters);

    // Value conversion helpers
    float normalizeOscillatorWaveform(const juce::String& waveform);
    float normalizeOscillatorOctave(int octave);
    float normalizeDetune(float cents);
    float normalizePan(float pan);
    float normalizeNoiseType(const juce::String& noiseType);
    float normalizeFilterType(const juce::String& filterType);
    float normalizeFilterRouting(const juce::String& routing);
    float normalizeFilterCutoff(float frequency);
    float normalizeFilterResonance(float q);
    float normalizeFilterGain(float gainDb);
    float normalizeLfoWaveform(const juce::String& waveform);
    float normalizeLfoRate(float hz);
    float normalizeLfoTarget(const juce::String& target);
    float normalizeTime(float seconds);

    // Reverse conversion for saving
    juce::String denormalizeOscillatorWaveform(float normalized);
    int denormalizeOscillatorOctave(float normalized);
    float denormalizeDetune(float normalized);
    float denormalizePan(float normalized);
    juce::String denormalizeNoiseType(float normalized);
    juce::String denormalizeFilterType(float normalized);
    juce::String denormalizeFilterRouting(float normalized);
    float denormalizeFilterCutoff(float normalized);
    float denormalizeFilterResonance(float normalized);
    float denormalizeFilterGain(float normalized);
    juce::String denormalizeLfoWaveform(float normalized);
    float denormalizeLfoRate(float normalized);
    juce::String denormalizeLfoTarget(float normalized);
    float denormalizeTime(float normalized);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JsonPresetManager)
};