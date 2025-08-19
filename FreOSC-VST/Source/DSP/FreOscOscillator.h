#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
    FreOSC Oscillator Class

    Wraps JUCE's DSP oscillator with additional functionality:
    - Multiple waveforms (Sine, Square, Sawtooth, Triangle)
    - Octave shifting
    - Fine detuning in cents
    - Level control
    - FM modulation support
*/
class FreOscOscillator
{
public:
    //==============================================================================
    // Waveform types matching JavaScript implementation
    enum class Waveform
    {
        Sine = 0,
        Square = 1,
        Sawtooth = 2,
        Triangle = 3
    };

    //==============================================================================
    FreOscOscillator();
    ~FreOscOscillator();

    //==============================================================================
    // Setup and configuration
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    //==============================================================================
    // Parameter control
    void setWaveform(Waveform waveform);
    void setFrequency(float frequency);
    void setLevel(float level); // 0.0 to 1.0
    void setOctave(int octave); // -2 to +2
    void setDetune(float cents); // -50 to +50 cents
    void setFrequencyModulation(float modAmount); // Real-time frequency modulation

    //==============================================================================
    // Processing
    float processSample(float fmInput = 0.0f);
    float processRawSample(float fmInput = 0.0f); // Raw waveform without level scaling (for PM)
    void processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, float fmInput = 0.0f);

    //==============================================================================
    // State queries
    bool isActive() const { return level > 0.0f; }
    float getCurrentLevel() const { return level; }
    float getCurrentFrequency() const { return finalFrequency; }
    Waveform getCurrentWaveform() const { return currentWaveform; }
    int getCurrentOctave() const { return octaveOffset; }
    float getCurrentDetune() const { return detuneAmount; }

private:
    //==============================================================================
    // Internal oscillator
    juce::dsp::Oscillator<float> oscillator;

    // Parameter state
    Waveform currentWaveform = Waveform::Sine;
    float baseFrequency = 440.0f;
    float finalFrequency = 440.0f;
    float level = 0.0f;
    int octaveOffset = 0;
    float detuneAmount = 0.0f;
    float frequencyModulation = 0.0f;

    // Audio processing
    double sampleRate = 44100.0;

    // Phase accumulator for FM synthesis
    float phase = 0.0f;
    float phaseIncrement = 0.0f;

    //==============================================================================
    // Helper methods
    void updateFinalFrequency();
    void updateOscillatorWaveform();
    float generateWaveformSample(float phaseValue) const;

    // Frequency calculation helpers
    static float centsToRatio(float cents);
    static float octaveToMultiplier(int octave);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscOscillator)
};

//==============================================================================
// Inline helper implementations

inline float FreOscOscillator::centsToRatio(float cents)
{
    // Convert cents to frequency ratio: ratio = 2^(cents/1200)
    return std::pow(2.0f, cents / 1200.0f);
}

inline float FreOscOscillator::octaveToMultiplier(int octave)
{
    // Convert octave offset to frequency multiplier: multiplier = 2^octave
    return std::pow(2.0f, static_cast<float>(octave));
}