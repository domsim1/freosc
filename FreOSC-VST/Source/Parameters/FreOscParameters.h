#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
/**
    FreOSC Parameter Definitions

    This class creates all the parameters for the FreOSC synthesizer plugin.
    It defines ranges, default values, and parameter layout to match the
    original JavaScript implementation.
*/
class FreOscParameters
{
public:
    //==============================================================================
    // Create the complete parameter layout for AudioProcessorValueTreeState
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // Waveform choices
    static const juce::StringArray oscillatorWaveforms;
    static const juce::StringArray noiseTypes;
    static const juce::StringArray filterTypes;
    static const juce::StringArray formantVowels;
    static const juce::StringArray fmSources;
    static const juce::StringArray fmTargets;
    static const juce::StringArray lfoWaveforms;
    static const juce::StringArray lfoTargets;

    //==============================================================================
    // Parameter ranges and defaults (matching JavaScript implementation)
    struct ParameterInfo
    {
        juce::String id;
        juce::String name;
        juce::NormalisableRange<float> range;
        float defaultValue;
        juce::String suffix;

        ParameterInfo(const juce::String& paramId,
                     const juce::String& paramName,
                     juce::NormalisableRange<float> paramRange,
                     float defaultVal,
                     const juce::String& paramSuffix = "")
            : id(paramId), name(paramName), range(paramRange),
              defaultValue(defaultVal), suffix(paramSuffix) {}
    };

    //==============================================================================
    // All parameter definitions
    static const std::vector<ParameterInfo> floatParameters;
    static const std::vector<std::tuple<juce::String, juce::String, juce::StringArray, int>> choiceParameters;

private:
    //==============================================================================
    // Helper functions for parameter creation
    static std::unique_ptr<juce::AudioParameterFloat> createFloatParameter(const ParameterInfo& info);
    static std::unique_ptr<juce::AudioParameterChoice> createChoiceParameter(
        const juce::String& id, const juce::String& name,
        const juce::StringArray& choices, int defaultIndex);
    static std::unique_ptr<juce::AudioParameterInt> createIntParameter(
        const juce::String& id, const juce::String& name,
        int min, int max, int defaultValue);
};

//==============================================================================
// Inline definitions for parameter arrays

// Oscillator waveform choices
inline const juce::StringArray FreOscParameters::oscillatorWaveforms = {
    "Sine", "Square", "Sawtooth", "Triangle"
};

// Noise type choices (matching JavaScript implementation)
inline const juce::StringArray FreOscParameters::noiseTypes = {
    "White", "Pink", "Brown", "Blue", "Violet", "Grey",
    "Crackle", "Digital", "Wind", "Ocean"
};

// Filter type choices
inline const juce::StringArray FreOscParameters::filterTypes = {
    "Low Pass", "High Pass", "Band Pass", "Low Shelf", "High Shelf",
    "Peaking", "Notch", "All Pass", "Formant"
};

// Formant vowel choices
inline const juce::StringArray FreOscParameters::formantVowels = {
    "A (ah)", "E (eh)", "I (ee)", "O (oh)", "U (oo)",
    "AE (ay)", "AW (aw)", "ER (ur)"
};

// FM source choices (not used - always Oscillator 3)
inline const juce::StringArray FreOscParameters::fmSources = {
    "Oscillator 3" // Always oscillator 3, no choice
};

// FM target choices - simplified to only target Osc 1, Osc 2, or both
inline const juce::StringArray FreOscParameters::fmTargets = {
    "Oscillator 1", "Oscillator 2", "Both Osc 1 & 2"
};

// LFO waveform choices
inline const juce::StringArray FreOscParameters::lfoWaveforms = {
    "Sine", "Triangle", "Sawtooth", "Square", "Random"
};

// LFO target choices
inline const juce::StringArray FreOscParameters::lfoTargets = {
    "None", "Pitch", "Filter Cutoff", "Volume", "Pan"
};

//==============================================================================
// Float parameter definitions with ranges matching JavaScript implementation
inline const std::vector<FreOscParameters::ParameterInfo> FreOscParameters::floatParameters = {
    // Oscillator 1 - Main oscillator, conservative for clean polyphony
    {"osc1_level",     "Osc1 Level",     {0.0f, 1.0f, 0.01f}, 0.3f},
    {"osc1_detune",    "Osc1 Detune",    {-50.0f, 50.0f, 1.0f}, 0.0f, " cents"},
    {"osc1_pan",       "Osc1 Pan",       {-1.0f, 1.0f, 0.01f}, 0.0f},

    // Oscillator 2 - Secondary oscillator with slight detune for richness
    {"osc2_level",     "Osc2 Level",     {0.0f, 1.0f, 0.01f}, 0.15f},
    {"osc2_detune",    "Osc2 Detune",    {-50.0f, 50.0f, 1.0f}, -7.0f, " cents"},
    {"osc2_pan",       "Osc2 Pan",       {-1.0f, 1.0f, 0.01f}, -0.2f},

    // Oscillator 3 - Lower octave for fullness, minimal level
    {"osc3_level",     "Osc3 Level",     {0.0f, 1.0f, 0.01f}, 0.05f},
    {"osc3_detune",    "Osc3 Detune",    {-50.0f, 50.0f, 1.0f}, 5.0f, " cents"},
    {"osc3_pan",       "Osc3 Pan",       {-1.0f, 1.0f, 0.01f}, 0.2f},

    // Noise - Off by default
    {"noise_level",    "Noise Level",    {0.0f, 1.0f, 0.01f}, 0.0f},
    {"noise_pan",      "Noise Pan",      {-1.0f, 1.0f, 0.01f}, 0.0f},

    // Master - Conservative default to prevent clipping in polyphonic use
    {"master_volume",  "Master Volume",  {0.0f, 1.0f, 0.01f}, 0.3f},

    // Envelope - Faster attack for immediate response, higher sustain for playability
    {"envelope_attack",  "Attack",   {0.0f, 2.0f, 0.01f}, 0.01f, " s"},
    {"envelope_decay",   "Decay",    {0.0f, 2.0f, 0.01f}, 0.2f, " s"},
    {"envelope_sustain", "Sustain",  {0.0f, 1.0f, 0.01f}, 0.8f},
    {"envelope_release", "Release",  {0.0f, 3.0f, 0.01f}, 0.3f, " s"},

    // Filter - Normalized values (0.0-1.0) for FreOscFilter compatibility
    {"filter_cutoff",    "Cutoff",     {0.0f, 1.0f, 0.01f}, 0.7f}, // Maps to 20Hz-20kHz, default ~7kHz
    {"filter_resonance", "Resonance",  {0.0f, 1.0f, 0.01f}, 0.03f}, // Maps to 0.1-30.0, default ~1.0
    {"filter_gain",      "Filter Gain", {0.0f, 1.0f, 0.01f}, 0.5f}, // Maps to -24dB to +24dB, default 0dB

    // FM Synthesis
    {"fm_amount",        "FM Amount",   {0.0f, 1000.0f, 1.0f}, 0.0f},
    {"fm_ratio",         "FM Ratio",    {0.1f, 8.0f, 0.1f}, 1.0f},

    // Dynamics removed - now uses fixed internal settings optimized for polyphony

    // Reverb - Subtle by default
    {"reverb_room_size", "Room Size",      {0.0f, 1.0f, 0.01f}, 0.3f},
    {"reverb_wet_level", "Reverb Wet",     {0.0f, 1.0f, 0.01f}, 0.1f},

    // Delay - Off by default for cleaner sound
    {"delay_time",       "Delay Time",     {0.0f, 1000.0f, 1.0f}, 250.0f, " ms"},
    {"delay_feedback",   "Delay Feedback", {0.0f, 0.95f, 0.01f}, 0.25f},
    {"delay_wet_level",  "Delay Wet",      {0.0f, 1.0f, 0.01f}, 0.0f},

    // LFO
    {"lfo_rate",         "LFO Rate",       {0.01f, 20.0f, 0.01f}, 2.0f, " Hz"},
    {"lfo_amount",       "LFO Amount",     {0.0f, 1.0f, 0.01f}, 0.0f}
};

//==============================================================================
// Choice and integer parameter definitions
inline const std::vector<std::tuple<juce::String, juce::String, juce::StringArray, int>> FreOscParameters::choiceParameters = {
    // Oscillator waveforms - Mix for rich sound
    {"osc1_waveform", "Osc1 Waveform", oscillatorWaveforms, 2}, // Sawtooth (bright lead)
    {"osc2_waveform", "Osc2 Waveform", oscillatorWaveforms, 1}, // Square (harmonic content)
    {"osc3_waveform", "Osc3 Waveform", oscillatorWaveforms, 0}, // Sine (sub bass)

    // Noise type
    {"noise_type", "Noise Type", noiseTypes, 0}, // White

    // Filter
    {"filter_type", "Filter Type", filterTypes, 0}, // Low Pass
    {"formant_vowel", "Formant Vowel", formantVowels, 0}, // A (ah)

    // FM - Source is always Oscillator 3, only target is selectable
    {"fm_source", "FM Source", fmSources, 0}, // Always Oscillator 3 (fixed)
    {"fm_target", "FM Target", fmTargets, 0}, // Oscillator 1

    // LFO
    {"lfo_waveform", "LFO Waveform", lfoWaveforms, 0}, // Sine
    {"lfo_target", "LFO Target", lfoTargets, 0} // None
};