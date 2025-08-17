#include "FreOscFilter.h"

//==============================================================================
// Formant frequency data - simplified and balanced for clean vocal synthesis
const FreOscFilter::FormantData FreOscFilter::formantTable[8] = {
    // Vowel A: "ah" sound - classic open vowel
    { 650.0f, 1080.0f, 2650.0f, 80.0f, 100.0f, 120.0f, 1.0f, 0.8f, 0.6f },
    // Vowel E: "eh" sound  
    { 400.0f, 2000.0f, 2800.0f, 70.0f, 110.0f, 130.0f, 1.0f, 0.9f, 0.7f },
    // Vowel I: "ee" sound
    { 300.0f, 2300.0f, 3200.0f, 60.0f, 120.0f, 140.0f, 1.0f, 0.9f, 0.8f },
    // Vowel O: "oh" sound
    { 450.0f, 850.0f, 2200.0f, 75.0f, 90.0f, 110.0f, 1.0f, 0.8f, 0.6f },
    // Vowel U: "oo" sound
    { 350.0f, 850.0f, 2200.0f, 65.0f, 85.0f, 105.0f, 1.0f, 0.7f, 0.5f },
    // Vowel AE: "ay" sound
    { 550.0f, 1900.0f, 2600.0f, 80.0f, 115.0f, 125.0f, 1.0f, 0.8f, 0.7f },
    // Vowel AW: "aw" sound
    { 600.0f, 1000.0f, 2400.0f, 85.0f, 95.0f, 115.0f, 1.0f, 0.7f, 0.6f },
    // Vowel ER: "ur" sound
    { 450.0f, 1200.0f, 1800.0f, 75.0f, 105.0f, 110.0f, 1.0f, 0.8f, 0.7f }
};

//==============================================================================
FreOscFilter::FreOscFilter()
{
}

FreOscFilter::~FreOscFilter()
{
}

//==============================================================================
void FreOscFilter::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    
    // Prepare main filter
    mainFilter.prepare(spec);
    
    // Initialize with current settings
    updateFilterCoefficients();
}

void FreOscFilter::reset()
{
    mainFilter.reset();
}

void FreOscFilter::process(const juce::dsp::ProcessContextReplacing<float>& context)
{
    // Simple unified processing - all filter types use the same path
    mainFilter.process(context);
}

//==============================================================================
void FreOscFilter::setFilterType(FilterType newType)
{
    if (currentFilterType != newType)
    {
        currentFilterType = newType;
        updateFilterCoefficients();
    }
}

void FreOscFilter::setCutoffFrequency(float normalizedFreq)
{
    normalizedFreq = juce::jlimit(0.0f, 1.0f, normalizedFreq);
    if (currentCutoffNormalized != normalizedFreq)
    {
        currentCutoffNormalized = normalizedFreq;
        updateFilterCoefficients();
    }
}

void FreOscFilter::setResonance(float normalizedQ)
{
    normalizedQ = juce::jlimit(0.0f, 1.0f, normalizedQ);
    if (currentResonanceNormalized != normalizedQ)
    {
        currentResonanceNormalized = normalizedQ;
        updateFilterCoefficients();
    }
}

void FreOscFilter::setGain(float normalizedGain)
{
    normalizedGain = juce::jlimit(0.0f, 1.0f, normalizedGain);
    if (currentGainNormalized != normalizedGain)
    {
        currentGainNormalized = normalizedGain;
        updateFilterCoefficients();
    }
}

void FreOscFilter::setFormantVowel(FormantVowel vowel)
{
    if (currentVowel != vowel)
    {
        currentVowel = vowel;
        if (currentFilterType == Formant)
        {
            updateFilterCoefficients();
        }
    }
}

//==============================================================================
void FreOscFilter::updateFilterCoefficients()
{
    auto coefficients = createFilterCoefficients();
    if (coefficients != nullptr)
    {
        *mainFilter.state = *coefficients;
    }
}

juce::dsp::IIR::Coefficients<float>::Ptr FreOscFilter::createFilterCoefficients()
{
    float freq = normalizedToFrequency(currentCutoffNormalized);
    float q = normalizedToQ(currentResonanceNormalized);
    float gainDb = normalizedToGainDb(currentGainNormalized);
    
    // Ensure frequency is within valid range
    freq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45), freq);
    q = juce::jmax(0.1f, q);
    
    switch (currentFilterType)
    {
        case Lowpass:
            return juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq, q);
            
        case Highpass:
            return juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, freq, q);
            
        case Bandpass:
            return juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, freq, q);
            
        case Notch:
            return juce::dsp::IIR::Coefficients<float>::makeNotch(sampleRate, freq, q);
            
        case Peaking:
            return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, freq, q, juce::Decibels::decibelsToGain(gainDb)
            );
            
        case Lowshelf:
            return juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sampleRate, freq, q, juce::Decibels::decibelsToGain(gainDb)
            );
            
        case Highshelf:
            return juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate, freq, q, juce::Decibels::decibelsToGain(gainDb)
            );
            
        case Allpass:
            return juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, freq, q);
            
        case Formant:
            {
                // Simple formant filter using first formant frequency with moderate boost
                const auto& formantData = formantTable[static_cast<int>(currentVowel)];
                float formantFreq = juce::jlimit(100.0f, static_cast<float>(sampleRate * 0.4), formantData.f1);
                float formantQ = formantFreq / formantData.bw1; // Q = freq/bandwidth
                formantQ = juce::jlimit(2.0f, 12.0f, formantQ);
                
                // Moderate vocal boost - 6-15dB based on gain setting
                float formantGainDb = 6.0f + (gainDb * 0.5f) + (formantData.gain1 * 9.0f);
                formantGainDb = juce::jlimit(6.0f, 18.0f, formantGainDb);
                
                return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                    sampleRate, formantFreq, formantQ, juce::Decibels::decibelsToGain(formantGainDb)
                );
            }
            
        default:
            return juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq, q);
    }
}

//==============================================================================
// Frequency conversion utilities with proper logarithmic scaling

float FreOscFilter::normalizedToFrequency(float normalized) const
{
    // Logarithmic scaling from 20Hz to 20kHz
    normalized = juce::jlimit(0.0f, 1.0f, normalized);
    return 20.0f * std::pow(1000.0f, normalized); // 20Hz * 1000^normalized = 20Hz to 20kHz
}

float FreOscFilter::normalizedToQ(float normalized) const
{
    // Linear scaling from 0.1 to 30.0
    normalized = juce::jlimit(0.0f, 1.0f, normalized);
    return 0.1f + (normalized * 29.9f);
}

float FreOscFilter::normalizedToGainDb(float normalized) const
{
    // Linear scaling from -24dB to +24dB
    normalized = juce::jlimit(0.0f, 1.0f, normalized);
    return -24.0f + (normalized * 48.0f);
}