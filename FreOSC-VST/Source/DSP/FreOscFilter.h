#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
    FreOSC Filter System - Rebuilt for reliability and LFO compatibility

    Clean implementation with proper frequency handling.
    All filter types work consistently with LFO modulation.

    Filter Types:
    0 - Lowpass: Standard low-pass filter
    1 - Highpass: Standard high-pass filter
    2 - Bandpass: Standard band-pass filter
*/
class FreOscFilter
{
public:
    //==============================================================================
    enum FilterType
    {
        Lowpass = 0,
        Highpass,
        Bandpass
    };

    //==============================================================================
    FreOscFilter();
    ~FreOscFilter();

    //==============================================================================
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(const juce::dsp::ProcessContextReplacing<float>& context);

    //==============================================================================
    // Parameter setters - all expect normalized values (0.0-1.0)
    void setFilterType(FilterType newType);
    void setCutoffFrequency(float normalizedFreq);  // 0.0-1.0 -> 20Hz-20kHz
    void setResonance(float normalizedQ);           // 0.0-1.0 -> 0.1-5.0
    void setGain(float normalizedGain);             // 0.0-1.0 -> -24dB to +24dB

    // Parameter getters
    FilterType getFilterType() const { return currentFilterType; }
    float getCutoffFrequency() const { return currentCutoffNormalized; }
    float getResonance() const { return currentResonanceNormalized; }
    float getGain() const { return currentGainNormalized; }

private:
    //==============================================================================
    // Filter parameters (all normalized 0.0-1.0)
    FilterType currentFilterType = Lowpass;
    float currentCutoffNormalized = 0.5f;     // 0.0-1.0
    float currentResonanceNormalized = 0.1f;  // 0.0-1.0
    float currentGainNormalized = 0.5f;       // 0.0-1.0
    double sampleRate = 44100.0;

    // Single unified filter for all types (stereo using ProcessorDuplicator)
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> mainFilter;

    //==============================================================================
    // Helper methods
    void updateFilterCoefficients();
    juce::dsp::IIR::Coefficients<float>::Ptr createFilterCoefficients();

    // Frequency conversion utilities
    float normalizedToFrequency(float normalized) const;
    float normalizedToQ(float normalized) const;
    float normalizedToGainDb(float normalized) const;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscFilter)
};