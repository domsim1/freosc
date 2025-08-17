#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
    FreOSC Noise Generator

    Generates various types of noise matching the JavaScript implementation:
    - White, Pink, Brown, Blue, Violet, Grey noise
    - Special types: Vinyl Crackle, Digital, Wind, Ocean

    Each noise type uses different algorithms to achieve specific
    frequency characteristics and textures.
*/
class FreOscNoiseGenerator
{
public:
    //==============================================================================
    // Noise types matching JavaScript implementation
    enum class NoiseType
    {
        White = 0,      // Equal power across all frequencies
        Pink = 1,       // 1/f frequency response
        Brown = 2,      // 1/f^2 frequency response (Brownian motion)
        Blue = 3,       // f frequency response (opposite of pink)
        Violet = 4,     // f^2 frequency response (opposite of brown)
        Grey = 5,       // Psychoacoustically flat noise
        Crackle = 6,    // Vinyl crackle - sparse pops and clicks
        Digital = 7,    // Quantized/aliased digital noise
        Wind = 8,       // Low frequency rumble with modulation
        Ocean = 9       // Band-pass filtered with wave-like modulation
    };

    //==============================================================================
    FreOscNoiseGenerator();
    ~FreOscNoiseGenerator();

    //==============================================================================
    // Setup and configuration
    void prepare(double sampleRate);
    void reset();

    //==============================================================================
    // Parameter control
    void setNoiseType(NoiseType type);
    void setLevel(float level); // 0.0 to 1.0
    void setPan(float pan);     // -1.0 to 1.0

    //==============================================================================
    // Processing
    float processSample();
    void processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    //==============================================================================
    // State queries
    bool isActive() const { return level > 0.0f; }
    float getCurrentLevel() const { return level; }
    NoiseType getCurrentType() const { return currentType; }

    //==============================================================================
    // Static utility functions
    static juce::String getNoiseTypeName(NoiseType type);
    static NoiseType getNoiseTypeFromIndex(int index);

private:
    //==============================================================================
    // Current state
    NoiseType currentType = NoiseType::White;
    float level = 0.0f;
    float pan = 0.0f;
    double sampleRate = 44100.0;

    // Random number generation
    juce::Random random;

    //==============================================================================
    // Pink noise filter states (pole-zero filter implementation)
    float pinkB0 = 0.0f, pinkB1 = 0.0f, pinkB2 = 0.0f;
    float pinkB3 = 0.0f, pinkB4 = 0.0f, pinkB5 = 0.0f, pinkB6 = 0.0f;

    // Brown noise state (integration)
    float brownState = 0.0f;

    // Blue noise state (differentiation)
    float blueLastOut = 0.0f;

    // Violet noise states (double differentiation)
    float violetLastOut = 0.0f;
    float violetLastOut2 = 0.0f;

    // Grey noise filter states (psychoacoustic shaping)
    float greyB0 = 0.0f, greyB1 = 0.0f, greyB2 = 0.0f;
    float greyB3 = 0.0f, greyB4 = 0.0f, greyB5 = 0.0f;

    // Digital noise states (quantization and aliasing)
    float digitalLastSample = 0.0f;

    // Wind noise states (heavy filtering with modulation)
    float windB0 = 0.0f, windB1 = 0.0f, windB2 = 0.0f;
    float windModPhase = 0.0f;

    // Ocean noise states (band-pass with wave modulation)
    float oceanB0 = 0.0f, oceanB1 = 0.0f, oceanB2 = 0.0f;
    float oceanWavePhase = 0.0f;
    float oceanWaveAmp = 0.0f;

    //==============================================================================
    // Noise generation methods (matching JavaScript algorithms exactly)
    float generateWhiteNoise();
    float generatePinkNoise();
    float generateBrownNoise();
    float generateBlueNoise();
    float generateVioletNoise();
    float generateGreyNoise();
    float generateCrackleNoise();
    float generateDigitalNoise();
    float generateWindNoise();
    float generateOceanNoise();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscNoiseGenerator)
};

//==============================================================================
// Inline implementations for simple functions

inline juce::String FreOscNoiseGenerator::getNoiseTypeName(NoiseType type)
{
    switch (type)
    {
        case NoiseType::White:   return "White";
        case NoiseType::Pink:    return "Pink";
        case NoiseType::Brown:   return "Brown";
        case NoiseType::Blue:    return "Blue";
        case NoiseType::Violet:  return "Violet";
        case NoiseType::Grey:    return "Grey";
        case NoiseType::Crackle: return "Crackle";
        case NoiseType::Digital: return "Digital";
        case NoiseType::Wind:    return "Wind";
        case NoiseType::Ocean:   return "Ocean";
        default: return "Unknown";
    }
}

inline FreOscNoiseGenerator::NoiseType FreOscNoiseGenerator::getNoiseTypeFromIndex(int index)
{
    if (index >= 0 && index <= 9)
        return static_cast<NoiseType>(index);
    return NoiseType::White;
}