#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
    FreOSC Filter System - Rebuilt for reliability and LFO compatibility

    Clean implementation with proper frequency handling and formant synthesis.
    All filter types work consistently with LFO modulation.

    Filter Types:
    0 - Lowpass: Standard low-pass filter
    1 - Highpass: Standard high-pass filter
    2 - Bandpass: Standard band-pass filter
    3 - Notch: Notch/band-stop filter
    4 - Peaking: Peaking EQ filter
    5 - Lowshelf: Low shelf EQ filter
    6 - Highshelf: High shelf EQ filter
    7 - Allpass: All-pass filter
    8 - Formant: Vocal formant filtering
*/
class FreOscFilter
{
public:
    //==============================================================================
    enum FilterType
    {
        Lowpass = 0,
        Highpass,
        Bandpass,
        Notch,
        Peaking,
        Lowshelf,
        Highshelf,
        Allpass,
        Formant
    };

    enum FormantVowel
    {
        A = 0,  // "ah" sound
        E,      // "eh" sound
        I,      // "ee" sound
        O,      // "oh" sound
        U,      // "oo" sound
        AE,     // "ay" sound
        AW,     // "aw" sound
        ER      // "ur" sound
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
    void setResonance(float normalizedQ);           // 0.0-1.0 -> 0.1-30.0
    void setGain(float normalizedGain);             // 0.0-1.0 -> -24dB to +24dB
    void setFormantVowel(FormantVowel vowel);

    // Parameter getters
    FilterType getFilterType() const { return currentFilterType; }
    float getCutoffFrequency() const { return currentCutoffNormalized; }
    float getResonance() const { return currentResonanceNormalized; }
    float getGain() const { return currentGainNormalized; }
    FormantVowel getFormantVowel() const { return currentVowel; }

private:
    //==============================================================================
    // Filter parameters (all normalized 0.0-1.0)
    FilterType currentFilterType = Lowpass;
    FormantVowel currentVowel = A;
    float currentCutoffNormalized = 0.5f;     // 0.0-1.0
    float currentResonanceNormalized = 0.1f;  // 0.0-1.0
    float currentGainNormalized = 0.5f;       // 0.0-1.0
    double sampleRate = 44100.0;

    // Single unified filter for all types (stereo using ProcessorDuplicator)
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> mainFilter;

    // Formant data structure
    struct FormantData
    {
        float f1, f2, f3;           // Formant frequencies
        float bw1, bw2, bw3;        // Bandwidths
        float gain1, gain2, gain3;  // Relative gains
    };

    static const FormantData formantTable[8];

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