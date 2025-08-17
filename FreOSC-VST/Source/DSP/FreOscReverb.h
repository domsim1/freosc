#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
    FreOSC Reverb Effect

    Complete convolution reverb implementation matching the FreOSC web version.
    Uses dynamic impulse response generation to simulate different room sizes
    with exponential decay and random noise characteristics.

    Features:
    - Convolution-based reverb processing
    - Dynamic impulse response generation
    - Room size parameter (0.0-1.0) controlling decay time and character
    - Wet/dry mixing with proper gain staging
    - Real-time parameter updates
*/
class FreOscReverb
{
public:
    //==============================================================================
    FreOscReverb();
    ~FreOscReverb();

    //==============================================================================
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(const juce::dsp::ProcessContextReplacing<float>& context);

    //==============================================================================
    // Parameter setters
    void setRoomSize(float roomSize);
    void setWetLevel(float wetLevel);

    // Parameter getters
    float getRoomSize() const { return currentRoomSize; }
    float getWetLevel() const { return currentWetLevel; }

private:
    //==============================================================================
    // Convolution processing
    juce::dsp::Convolution convolution;

    // Wet/dry mixing
    juce::dsp::Gain<float> wetGain;
    juce::dsp::Gain<float> dryGain;

    // Parameter state
    float currentRoomSize = 0.5f;
    float currentWetLevel = 0.2f;
    double sampleRate = 44100.0;

    // Impulse response management
    juce::AudioBuffer<float> currentImpulse;
    bool needsImpulseUpdate = true;

    // Algorithmic reverb components
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLines[8];
    std::array<float, 8> delayTimes = {{ 0.023f, 0.031f, 0.041f, 0.053f, 0.067f, 0.079f, 0.089f, 0.103f }};
    std::array<float, 8> feedback = {{ 0.3f, 0.28f, 0.25f, 0.23f, 0.2f, 0.18f, 0.15f, 0.12f }};
    juce::dsp::IIR::Filter<float> dampingFilter;

    //==============================================================================
    // Impulse response generation (matching JavaScript implementation)
    void updateImpulseResponse();
    juce::AudioBuffer<float> generateImpulseResponse(float duration, float decay, bool reverse = false);

    // Parameter update helpers
    void updateMixLevels();

    // Simple algorithmic reverb processing
    void processAlgorithmicReverb(const juce::dsp::ProcessContextReplacing<float>& context);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscReverb)
};