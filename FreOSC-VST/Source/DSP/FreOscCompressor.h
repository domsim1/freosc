#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
    Clean, Musical Compressor with Smooth RMS Detection
    
    Features:
    - Smooth RMS level detection with adjustable ballistics
    - Soft-knee compression for transparent operation
    - Musical attack/release curves
    - Automatic makeup gain compensation
    - Sidechain high-pass filtering to reduce pumping
    - Clean, artifact-free processing
*/
class FreOscCompressor
{
public:
    //==============================================================================
    FreOscCompressor();
    ~FreOscCompressor();
    
    //==============================================================================
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    
    //==============================================================================
    // Main processing
    template<typename ProcessContext>
    void process(const ProcessContext& context) noexcept
    {
        auto& inputBlock = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        
        jassert(inputBlock.getNumChannels() == outputBlock.getNumChannels());
        jassert(inputBlock.getNumSamples() == outputBlock.getNumSamples());
        
        if (context.isBypassed)
        {
            outputBlock.copyFrom(inputBlock);
            return;
        }
        
        auto numChannels = static_cast<int>(inputBlock.getNumChannels());
        auto numSamples = static_cast<int>(inputBlock.getNumSamples());
        
        // Process each sample
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Get input samples
            float inputL = inputBlock.getSample(0, static_cast<size_t>(sample));
            float inputR = numChannels > 1 ? inputBlock.getSample(1, static_cast<size_t>(sample)) : inputL;
            
            // Apply compression
            auto result = processStereoSample(inputL, inputR);
            
            // Set output samples
            outputBlock.setSample(0, static_cast<size_t>(sample), result.first);
            if (numChannels > 1)
                outputBlock.setSample(1, static_cast<size_t>(sample), result.second);
        }
    }
    
    //==============================================================================
    // Parameter setters
    void setThreshold(float thresholdDb);      // -60 to 0 dB
    void setRatio(float ratio);                // 1:1 to 20:1
    void setAttack(float attackMs);            // 0.1 to 100 ms
    void setRelease(float releaseMs);          // 10 to 1000 ms
    void setKnee(float kneeDb);                // 0 to 10 dB (soft knee width)
    void setMakeupGain(float gainDb);          // -20 to +20 dB
    void setMix(float mix);                    // 0 to 1 (dry/wet)
    
    //==============================================================================
    // Metering
    float getCurrentGainReduction() const { return currentGainReduction; }
    float getCurrentInputLevel() const { return currentInputLevel; }
    float getCurrentOutputLevel() const { return currentOutputLevel; }
    
private:
    //==============================================================================
    // Parameters
    float threshold = -12.0f;      // dB
    float ratio = 4.0f;            // ratio
    float attackTime = 1.0f;       // ms
    float releaseTime = 100.0f;    // ms  
    float kneeWidth = 2.0f;        // dB
    float makeupGain = 0.0f;       // dB
    float mixAmount = 1.0f;        // 0-1
    
    // Processing state
    double sampleRate = 44100.0;
    float currentGainReduction = 0.0f;
    float currentInputLevel = 0.0f;
    float currentOutputLevel = 0.0f;
    
    // Smooth level detection (RMS with ballistics)
    juce::LinearSmoothedValue<float> rmsLevel{0.0f};
    juce::LinearSmoothedValue<float> gainReductionSmooth{0.0f};
    
    // Sidechain high-pass filter to reduce pumping on bass
    juce::dsp::IIR::Filter<float> sidechainHPF_L, sidechainHPF_R;
    
    // Makeup gain smoother
    juce::LinearSmoothedValue<float> makeupGainSmooth{0.0f};
    
    //==============================================================================
    // Core processing methods
    std::pair<float, float> processStereoSample(float inputL, float inputR);
    float calculateGainReduction(float inputLevel);
    float softKneeCompression(float inputLevel, float threshold, float ratio, float kneeWidth);
    float dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }
    float linearToDb(float linear) { return 20.0f * std::log10(juce::jmax(linear, 1e-10f)); }
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscCompressor)
};