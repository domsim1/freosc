#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

//==============================================================================
/**
    FreOSC Wavefolder Distortion Effect
    
    Implements classic wavefolder distortion that creates complex harmonic content
    by "folding" the waveform when it exceeds threshold levels. This creates
    the characteristic bright, aggressive sound of analog wavefolders.
    
    Features:
    - Drive: Input gain control (0-100%)
    - Threshold: Folding threshold (0-100%)
    - Symmetry: Asymmetric vs symmetric folding (0-100%)
    - Mix: Dry/wet mix control (0-100%)
    - Output Level: Post-processing gain compensation (0-100%)
*/
class FreOscWavefolder
{
public:
    //==============================================================================
    FreOscWavefolder();
    ~FreOscWavefolder() = default;

    //==============================================================================
    // Setup and configuration
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    //==============================================================================
    // Parameter control methods
    void setDrive(float drive);           // 0.0 to 1.0 (maps to 1x to 10x gain)
    void setThreshold(float threshold);   // 0.0 to 1.0 (folding threshold)
    void setSymmetry(float symmetry);     // 0.0 to 1.0 (0 = symmetric, 1 = asymmetric)
    void setMix(float mix);               // 0.0 to 1.0 (dry/wet)
    void setOutputLevel(float level);     // 0.0 to 1.0 (output gain compensation)

    //==============================================================================
    // Processing
    template<typename ProcessContext>
    void process(const ProcessContext& context)
    {
        auto& inputBlock = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        
        auto numChannels = inputBlock.getNumChannels();
        auto numSamples = inputBlock.getNumSamples();

        // Process each channel
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* inputSamples = inputBlock.getChannelPointer(ch);
            auto* outputSamples = outputBlock.getChannelPointer(ch);

            for (size_t sample = 0; sample < numSamples; ++sample)
            {
                float inputSample = inputSamples[sample];
                float processedSample = processSample(inputSample, static_cast<int>(ch));
                
                // Apply dry/wet mix
                outputSamples[sample] = (inputSample * (1.0f - mix)) + (processedSample * mix);
            }
        }
    }

private:
    //==============================================================================
    // Internal processing methods
    float processSample(float sample, int channel);
    float wavefold(float sample);

    //==============================================================================
    // Parameters
    float drive = 1.0f;
    float threshold = 0.7f;
    float symmetry = 0.0f;  // 0 = symmetric, 1 = full asymmetric
    float mix = 1.0f;       // 0 = dry, 1 = wet
    float outputLevel = 0.5f;

    //==============================================================================
    // State variables
    double sampleRate = 44100.0;
    
    // DC blocking filters (one per channel)
    std::vector<juce::dsp::IIR::Filter<float>> dcBlockers;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscWavefolder)
};