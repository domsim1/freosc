#include "FreOscFilter.h"


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
    // First apply the IIR filtering
    mainFilter.process(context);
    
    // Then apply filter gain (if not unity gain)
    float gainDb = normalizedToGainDb(currentGainNormalized);
    if (std::abs(gainDb) > 0.1f) // Only apply gain if it's not near 0dB
    {
        float linearGain = juce::Decibels::decibelsToGain(gainDb);
        
        // Apply gain to all channels and samples
        auto& audioBlock = context.getOutputBlock();
        for (size_t channel = 0; channel < audioBlock.getNumChannels(); ++channel)
        {
            auto* channelData = audioBlock.getChannelPointer(channel);
            for (size_t sample = 0; sample < audioBlock.getNumSamples(); ++sample)
            {
                channelData[sample] *= linearGain;
            }
        }
    }
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
    if (std::abs(currentCutoffNormalized - normalizedFreq) > 1e-6f)
    {
        currentCutoffNormalized = normalizedFreq;
        updateFilterCoefficients();
    }
}

void FreOscFilter::setResonance(float normalizedQ)
{
    normalizedQ = juce::jlimit(0.0f, 1.0f, normalizedQ);
    if (std::abs(currentResonanceNormalized - normalizedQ) > 1e-6f)
    {
        currentResonanceNormalized = normalizedQ;
        updateFilterCoefficients();
    }
}

void FreOscFilter::setGain(float normalizedGain)
{
    normalizedGain = juce::jlimit(0.0f, 1.0f, normalizedGain);
    if (std::abs(currentGainNormalized - normalizedGain) > 1e-6f)
    {
        currentGainNormalized = normalizedGain;
        updateFilterCoefficients();
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

    // Ensure frequency is within valid range
    freq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45), freq);
    q = juce::jmax(0.1f, q);

    switch (currentFilterType)
    {
        case Lowpass:
            // Low-pass filters are prone to instability at high Q - limit to 2.5 for clean sound
            q = juce::jmin(q, 2.5f);
            // Also ensure minimum frequency when Q is high to prevent very low freq + high Q instability
            if (q > 2.0f && freq < 50.0f)
                freq = 50.0f;
            return juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq, q);

        case Highpass:
            return juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, freq, q);

        case Bandpass:
            return juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, freq, q);

        case Notch:
            return juce::dsp::IIR::Coefficients<float>::makeNotch(sampleRate, freq, q);

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
    // Linear scaling from 0.1 to 5.0
    normalized = juce::jlimit(0.0f, 1.0f, normalized);
    return 0.1f + (normalized * 4.9f);
}

float FreOscFilter::normalizedToGainDb(float normalized) const
{
    // Linear scaling from -24dB to +24dB
    normalized = juce::jlimit(0.0f, 1.0f, normalized);
    return -24.0f + (normalized * 48.0f);
}