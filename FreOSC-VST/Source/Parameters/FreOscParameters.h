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
    static const juce::StringArray filterRouting;
    static const juce::StringArray effectsRouting;
    static const juce::StringArray pmCarriers;
    static const juce::StringArray lfoWaveforms;
    static const juce::StringArray lfoTargets;
    static const juce::StringArray modEnvelopeTargets;

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
    "Low Pass", "High Pass", "Band Pass"
};

// Filter routing choices
inline const juce::StringArray FreOscParameters::filterRouting = {
    "Filter 1 Only", "Parallel", "Series"
};

// Effects routing choices
inline const juce::StringArray FreOscParameters::effectsRouting = {
    "Series Reverb to Delay", "Series Delay to Reverb", "Parallel"
};


// PM carrier choices - which oscillators receive phase modulation from OSC3
inline const juce::StringArray FreOscParameters::pmCarriers = {
    "Oscillator 1", "Oscillator 2", "Both Osc 1 & 2"
};

// LFO waveform choices
inline const juce::StringArray FreOscParameters::lfoWaveforms = {
    "Sine", "Triangle", "Sawtooth", "Square", "Random"
};

// LFO target choices
inline const juce::StringArray FreOscParameters::lfoTargets = {
    "None", "Pitch", "Filter Cutoff", "Filter2 Cutoff", "Volume", "Pan"
};

// Modulation envelope target choices
inline const juce::StringArray FreOscParameters::modEnvelopeTargets = {
    "None", "PM Index", "PM Ratio", "Filter Cutoff", "Filter2 Cutoff"
};

//==============================================================================
// Float parameter definitions with ranges matching JavaScript implementation
inline const std::vector<FreOscParameters::ParameterInfo> FreOscParameters::floatParameters = {
    // Oscillator 1 - Simple sine wave
    {"osc1_level",     "Osc1 Level",     {0.0f, 1.0f, 0.01f}, 0.5f},
    {"osc1_detune",    "Osc1 Detune",    {-50.0f, 50.0f, 1.0f}, 0.0f, " cents"},
    {"osc1_pan",       "Osc1 Pan",       {-1.0f, 1.0f, 0.01f}, 0.0f},

    // Oscillator 2 - Off by default
    {"osc2_level",     "Osc2 Level",     {0.0f, 1.0f, 0.01f}, 0.0f},
    {"osc2_detune",    "Osc2 Detune",    {-50.0f, 50.0f, 1.0f}, 0.0f, " cents"},
    {"osc2_pan",       "Osc2 Pan",       {-1.0f, 1.0f, 0.01f}, 0.0f},

    // Oscillator 3 - Off by default
    {"osc3_level",     "Osc3 Level",     {0.0f, 1.0f, 0.01f}, 0.0f},
    {"osc3_detune",    "Osc3 Detune",    {-50.0f, 50.0f, 1.0f}, 0.0f, " cents"},
    {"osc3_pan",       "Osc3 Pan",       {-1.0f, 1.0f, 0.01f}, 0.0f},

    // Noise - Off by default
    {"noise_level",    "Noise Level",    {0.0f, 1.0f, 0.01f}, 0.0f},
    {"noise_pan",      "Noise Pan",      {-1.0f, 1.0f, 0.01f}, 0.0f},

    // Master - Simple moderate volume
    {"master_volume",  "Master Volume",  {0.0f, 1.0f, 0.01f}, 0.5f},

    // Envelope - Simple organ-style envelope
    {"envelope_attack",  "Attack",   {0.0f, 2.0f, 0.01f}, 0.0f, " s"},
    {"envelope_decay",   "Decay",    {0.0f, 2.0f, 0.01f}, 0.0f, " s"},
    {"envelope_sustain", "Sustain",  {0.0f, 1.0f, 0.01f}, 1.0f},
    {"envelope_release", "Release",  {0.0f, 3.0f, 0.01f}, 0.1f, " s"},

    // Filter - Wide open for clean sound
    {"filter_cutoff",    "Cutoff",     {0.0f, 1.0f, 0.01f}, 1.0f}, // Wide open (20kHz)
    {"filter_resonance", "Resonance",  {0.0f, 1.0f, 0.01f}, 0.0f}, // Minimal resonance
    {"filter_gain",      "Filter Gain", {0.0f, 1.0f, 0.01f}, 0.5f}, // Neutral gain (0dB)

    // Filter 2 - Wide open
    {"filter2_cutoff",    "Filter2 Cutoff",     {0.0f, 1.0f, 0.01f}, 1.0f}, // Wide open (20kHz)
    {"filter2_resonance", "Filter2 Resonance",  {0.0f, 1.0f, 0.01f}, 0.0f}, // Minimal resonance
    {"filter2_gain",      "Filter2 Gain",       {0.0f, 1.0f, 0.01f}, 0.5f}, // Neutral gain (0dB)

    // PM Synthesis (Phase Modulation)
    {"pm_index",         "PM Index",    {0.0f, 10.0f, 0.01f}, 0.0f},
    {"pm_ratio",         "PM Ratio",    {0.1f, 8.0f, 0.1f}, 1.0f},

    // Dynamics removed - now uses fixed internal settings optimized for polyphony

    // Plate Reverb - Off by default
    {"plate_predelay",   "Pre-Delay",      {0.0f, 1.0f, 0.01f}, 0.0f},
    {"plate_size",       "Size",           {0.0f, 1.0f, 0.01f}, 0.0f},
    {"plate_damping",    "Damping",        {0.0f, 1.0f, 0.01f}, 0.5f},
    {"plate_diffusion",  "Diffusion",      {0.0f, 1.0f, 0.01f}, 0.5f},
    {"plate_wet_level",  "Plate Wet",      {0.0f, 1.0f, 0.01f}, 0.0f},
    {"plate_width",      "Stereo Width",   {0.0f, 1.0f, 0.01f}, 0.5f},

    // Tape Delay - Off by default
    {"tape_time",        "Tape Time",      {0.0f, 1.0f, 0.01f}, 0.0f},
    {"tape_feedback",    "Tape Feedback",  {0.0f, 1.0f, 0.01f}, 0.0f},
    {"tape_tone",        "Tape Tone",      {0.0f, 1.0f, 0.01f}, 0.5f},
    {"tape_flutter",     "Tape Flutter",   {0.0f, 1.0f, 0.01f}, 0.0f},
    {"tape_wet_level",   "Tape Wet",       {0.0f, 1.0f, 0.01f}, 0.0f},
    {"tape_width",       "Tape Width",     {0.0f, 1.0f, 0.01f}, 0.5f},

    // LFO
    {"lfo_rate",         "LFO Rate",       {0.01f, 20.0f, 0.01f}, 2.0f, " Hz"},
    {"lfo_amount",       "LFO Amount",     {0.0f, 1.0f, 0.01f}, 0.0f},

    // Modulation Envelope 1
    {"mod_env1_attack",  "ModEnv1 Attack",  {0.0f, 2.0f, 0.01f}, 0.01f, " s"},
    {"mod_env1_decay",   "ModEnv1 Decay",   {0.0f, 2.0f, 0.01f}, 0.2f, " s"},
    {"mod_env1_sustain", "ModEnv1 Sustain", {0.0f, 1.0f, 0.01f}, 0.8f},
    {"mod_env1_release", "ModEnv1 Release", {0.0f, 3.0f, 0.01f}, 0.3f, " s"},
    {"mod_env1_amount",  "ModEnv1 Amount",  {0.0f, 1.0f, 0.01f}, 0.0f},

    // Modulation Envelope 2
    {"mod_env2_attack",  "ModEnv2 Attack",  {0.0f, 2.0f, 0.01f}, 0.01f, " s"},
    {"mod_env2_decay",   "ModEnv2 Decay",   {0.0f, 2.0f, 0.01f}, 0.2f, " s"},
    {"mod_env2_sustain", "ModEnv2 Sustain", {0.0f, 1.0f, 0.01f}, 0.8f},
    {"mod_env2_release", "ModEnv2 Release", {0.0f, 3.0f, 0.01f}, 0.3f, " s"},
    {"mod_env2_amount",  "ModEnv2 Amount",  {0.0f, 1.0f, 0.01f}, 0.0f}
};

//==============================================================================
// Choice and integer parameter definitions
inline const std::vector<std::tuple<juce::String, juce::String, juce::StringArray, int>> FreOscParameters::choiceParameters = {
    // Oscillator waveforms - Simple sine waves
    {"osc1_waveform", "Osc1 Waveform", oscillatorWaveforms, 0}, // Sine
    {"osc2_waveform", "Osc2 Waveform", oscillatorWaveforms, 0}, // Sine
    {"osc3_waveform", "Osc3 Waveform", oscillatorWaveforms, 0}, // Sine

    // Noise type
    {"noise_type", "Noise Type", noiseTypes, 0}, // White

    // Filter
    {"filter_type", "Filter Type", filterTypes, 0}, // Low Pass
    {"filter2_type", "Filter2 Type", filterTypes, 2}, // Band Pass for complementary filtering
    {"filter_routing", "Filter Routing", filterRouting, 0}, // Off
    {"effects_routing", "Effects Routing", effectsRouting, 0}, // Series Reverb to Delay

    // PM - OSC3 is always the message signal, user selects carrier(s)
    {"pm_carrier", "PM Carrier", pmCarriers, 0}, // Oscillator 1

    // LFO
    {"lfo_waveform", "LFO Waveform", lfoWaveforms, 0}, // Sine
    {"lfo_target", "LFO Target", lfoTargets, 0}, // None

    // Modulation Envelopes
    {"mod_env1_target", "ModEnv1 Target", modEnvelopeTargets, 0}, // None
    {"mod_env2_target", "ModEnv2 Target", modEnvelopeTargets, 0}  // None
};