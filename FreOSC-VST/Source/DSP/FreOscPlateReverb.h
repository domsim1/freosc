#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

/**
 * FreOscPlateReverb - EMT plate reverb simulation
 * 
 * Features:
 * - Classic EMT plate reverb characteristics
 * - Pre-delay for spatial positioning  
 * - Damping control for high frequency rolloff
 * - Diffusion control for density/texture
 * - Size control for decay time
 * - Wet/dry mixing
 * - Stereo width control
 */
class FreOscPlateReverb
{
public:
    //==============================================================================
    FreOscPlateReverb();
    ~FreOscPlateReverb();

    //==============================================================================
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(const juce::dsp::ProcessContextReplacing<float>& context);

    //==============================================================================
    // Parameter setters (0.0 to 1.0 normalized range)
    void setPreDelay(float preDelay);      // 0ms to 250ms
    void setSize(float size);              // Decay time multiplier
    void setDamping(float damping);        // High frequency damping
    void setDiffusion(float diffusion);    // Density/texture control
    void setWetLevel(float wetLevel);      // Wet signal level
    void setStereoWidth(float width);      // Stereo spread (0=mono, 1=full stereo)

private:
    //==============================================================================
    // Core plate reverb algorithm
    struct DelayLine
    {
        std::vector<float> buffer;
        int writePos = 0;
        int size = 0;
        
        void setSize(int newSize);
        void clear();
        float read(int delaySamples) const;
        void write(float sample);
        float readInterpolated(float delaySamples) const;
    };
    
    struct AllpassFilter
    {
        DelayLine delay;
        float gain = 0.7f;
        
        void setDelay(int samples);
        float process(float input);
    };
    
    struct CombFilter  
    {
        DelayLine delay;
        float feedback = 0.5f;
        float dampingGain = 0.2f;
        float lastOutput = 0.0f;
        
        void setDelay(int samples);
        void setFeedback(float fb);
        void setDamping(float damp);
        float process(float input);
    };
    
    //==============================================================================
    // Plate reverb network structure
    static constexpr int numAllpass = 4;
    static constexpr int numCombs = 8;
    
    // Pre-delay line
    DelayLine preDelayLine;
    
    // Allpass diffusers (create initial diffusion)
    AllpassFilter allpass[numAllpass];
    
    // Parallel comb filters (main decay structure)
    CombFilter combsL[numCombs / 2];  // Left channel combs
    CombFilter combsR[numCombs / 2];  // Right channel combs
    
    // High frequency damping filters
    juce::dsp::IIR::Filter<float> dampingFilterL, dampingFilterR;
    
    // Stereo processing is handled inline in process method
    
    // Gain stages
    juce::dsp::Gain<float> wetGain, dryGain;
    
    //==============================================================================
    // Parameters
    double sampleRate = 44100.0;
    
    float currentPreDelay = 0.1f;      // 0.0 to 1.0
    float currentSize = 0.5f;          // 0.0 to 1.0  
    float currentDamping = 0.3f;       // 0.0 to 1.0
    float currentDiffusion = 0.7f;     // 0.0 to 1.0
    float currentWetLevel = 0.2f;      // 0.0 to 1.0
    float currentStereoWidth = 0.8f;   // 0.0 to 1.0
    
    //==============================================================================
    // Helper methods
    void updateDelayTimes();
    void updateFeedback();
    void updateDamping();
    void updateStereoMatrix();
    void updateMixLevels();
    
    // EMT plate delay time constants (in samples at 44.1kHz)
    static constexpr int baseAllpassDelays[numAllpass] = { 347, 113, 37, 59 };
    static constexpr int baseCombDelays[numCombs] = { 
        1687, 1601, 2053, 2251,  // Left channel
        1733, 1667, 2089, 2203   // Right channel  
    };
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscPlateReverb)
};