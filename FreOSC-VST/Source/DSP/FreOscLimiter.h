#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
    Clean, Musical Limiter with Lookahead and Soft Saturation
    
    Features:
    - Lookahead peak detection for clean limiting
    - Soft saturation before hard limiting for musical character
    - True peak limiting with oversampling
    - Smooth gain reduction envelope
    - ISR (Intersample Peak) detection
    - Clean, artifact-free processing
*/
class FreOscLimiter
{
public:
    //==============================================================================
    FreOscLimiter();
    ~FreOscLimiter();
    
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
            
            // Apply limiting
            auto result = processStereoSample(inputL, inputR);
            
            // Set output samples
            outputBlock.setSample(0, static_cast<size_t>(sample), result.first);
            if (numChannels > 1)
                outputBlock.setSample(1, static_cast<size_t>(sample), result.second);
        }
    }
    
    //==============================================================================
    // Parameter setters
    void setThreshold(float thresholdDb);      // -20 to 0 dB
    void setRelease(float releaseMs);          // 1 to 1000 ms
    void setCeiling(float ceilingDb);          // -1 to 0 dB (output ceiling)
    void setSaturation(float amount);          // 0 to 1 (soft saturation amount)
    void setLookahead(float lookaheadMs);      // 0 to 10 ms
    
    //==============================================================================
    // Metering
    float getCurrentGainReduction() const { return currentGainReduction; }
    float getCurrentInputLevel() const { return currentInputLevel; }
    float getCurrentOutputLevel() const { return currentOutputLevel; }
    bool isLimiting() const { return currentGainReduction > 0.1f; }
    
private:
    //==============================================================================
    // Parameters
    float threshold = -3.0f;        // dB
    float releaseTime = 50.0f;      // ms
    float ceiling = -0.1f;          // dB
    float saturationAmount = 0.3f;  // 0-1
    float lookaheadTime = 2.0f;     // ms
    
    // Processing state
    double sampleRate = 44100.0;
    float currentGainReduction = 0.0f;
    float currentInputLevel = 0.0f;
    float currentOutputLevel = 0.0f;
    
    // Lookahead delay buffer
    static constexpr int maxLookaheadSamples = 1024; // ~23ms at 44.1kHz
    juce::AudioBuffer<float> lookaheadBuffer;
    int lookaheadSamples = 0;
    int bufferWritePos = 0;
    
    // Peak detection and gain reduction
    juce::LinearSmoothedValue<float> gainReductionSmooth{0.0f};
    float peakHoldLevel = 0.0f;
    int peakHoldCounter = 0;
    static constexpr int peakHoldTime = 32; // samples
    
    // Soft saturation handled manually in processStereoSample
    
    // DC blocker to remove any DC offset from saturation
    juce::dsp::IIR::Filter<float> dcBlockerL, dcBlockerR;
    
    //==============================================================================
    // Core processing methods
    std::pair<float, float> processStereoSample(float inputL, float inputR);
    float detectPeak(float sample);
    float calculateGainReduction(float peakLevel);
    float softSaturate(float input, float amount);
    static float tanhSaturation(float input) { return std::tanh(input); }
    float dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }
    float linearToDb(float linear) { return 20.0f * std::log10(juce::jmax(linear, 1e-10f)); }
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscLimiter)
};