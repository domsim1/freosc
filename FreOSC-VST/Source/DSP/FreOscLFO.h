#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
    FreOSC Low Frequency Oscillator

    Provides modulation for various synthesis parameters:
    - 5 waveform types (Sine, Triangle, Sawtooth, Square, Random)
    - 4 modulation targets (Pitch, Filter, Volume, Pan)
    - Rate control from 0.01 to 20 Hz
    - Amount control for modulation depth
*/
class FreOscLFO
{
public:
    //==============================================================================
    // LFO waveform types matching JavaScript implementation
    enum class Waveform
    {
        Sine = 0,
        Triangle = 1,
        Sawtooth = 2,
        Square = 3,
        Random = 4  // Sample and hold random
    };

    // LFO modulation targets
    enum class Target
    {
        None = 0,
        Pitch = 1,
        Filter = 2,
        Volume = 3,
        Pan = 4
    };

    //==============================================================================
    FreOscLFO();
    ~FreOscLFO();

    //==============================================================================
    // Setup and configuration
    void prepare(double sampleRate);
    void reset();

    //==============================================================================
    // Parameter control
    void setWaveform(Waveform waveform);
    void setRate(float rateHz);          // 0.01 to 20 Hz
    void setTarget(Target target);
    void setAmount(float amount);        // 0.0 to 1.0

    //==============================================================================
    // Processing
    float getNextSample(Waveform waveform, float rate, Target target);

    //==============================================================================
    // State queries
    bool isActive() const { return amount > 0.0f && currentTarget != Target::None; }
    float getCurrentRate() const { return rate; }
    float getCurrentAmount() const { return amount; }
    Waveform getCurrentWaveform() const { return currentWaveform; }
    Target getCurrentTarget() const { return currentTarget; }

    //==============================================================================
    // Utility functions
    static juce::String getWaveformName(Waveform waveform);
    static juce::String getTargetName(Target target);

private:
    //==============================================================================
    // LFO state
    Waveform currentWaveform = Waveform::Sine;
    Target currentTarget = Target::None;
    float rate = 2.0f;          // Hz
    float amount = 0.0f;        // 0.0 to 1.0

    // Audio processing
    double sampleRate = 44100.0;

    // Oscillator state
    juce::dsp::Oscillator<float> oscillator;

    // Random LFO (sample and hold)
    juce::Random random;
    float randomValue = 0.0f;
    int samplesSinceLastRandom = 0;
    int samplesPerRandomStep = 0;

    // Phase tracking for manual waveform generation
    float phase = 0.0f;
    float phaseIncrement = 0.0f;

    //==============================================================================
    // Waveform generation methods
    float generateSine();
    float generateTriangle();
    float generateSawtooth();
    float generateSquare();
    float generateRandom();

    void updatePhaseIncrement();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscLFO)
};

//==============================================================================
// Inline utility functions

inline juce::String FreOscLFO::getWaveformName(Waveform waveform)
{
    switch (waveform)
    {
        case Waveform::Sine:     return "Sine";
        case Waveform::Triangle: return "Triangle";
        case Waveform::Sawtooth: return "Sawtooth";
        case Waveform::Square:   return "Square";
        case Waveform::Random:   return "Random";
        default: return "Unknown";
    }
}

inline juce::String FreOscLFO::getTargetName(Target target)
{
    switch (target)
    {
        case Target::None:   return "None";
        case Target::Pitch:  return "Pitch";
        case Target::Filter: return "Filter Cutoff";
        case Target::Volume: return "Volume";
        case Target::Pan:    return "Pan";
        default: return "Unknown";
    }
}