#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
    FreOSC Delay Effect
    
    Complete digital delay implementation matching the FreOSC web version.
    Features delay line with feedback control and wet/dry mixing for
    creating echo, slap-back, and ambient delay effects.
    
    Features:
    - Digital delay line with up to 1 second maximum delay
    - Feedback control with stability limiting
    - Independent wet/dry level control
    - Real-time parameter updates
    - High-quality interpolation for smooth parameter changes
*/
class FreOscDelay
{
public:
    //==============================================================================
    FreOscDelay();
    ~FreOscDelay();
    
    //==============================================================================
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(const juce::dsp::ProcessContextReplacing<float>& context);
    
    //==============================================================================
    // Parameter setters
    void setDelayTime(float delayTimeMs);
    void setFeedback(float feedback);
    void setWetLevel(float wetLevel);
    
    // Parameter getters
    float getDelayTime() const { return currentDelayTime; }
    float getFeedback() const { return currentFeedback; }
    float getWetLevel() const { return currentWetLevel; }
    
private:
    //==============================================================================
    // Delay processing components
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    
    // Wet/dry mixing and feedback
    juce::dsp::Gain<float> wetGain;
    juce::dsp::Gain<float> dryGain;
    juce::dsp::Gain<float> feedbackGain;
    
    // Parameter state
    float currentDelayTime = 250.0f;  // milliseconds
    float currentFeedback = 0.3f;     // 0.0 to 0.95 max for stability
    float currentWetLevel = 0.2f;     // 0.0 to 1.0
    double sampleRate = 44100.0;
    
    // Parameter smoothing for artifact-free changes
    juce::SmoothedValue<float> smoothedDelayTime;
    
    //==============================================================================
    // Helper methods
    void updateDelayParameters();
    void updateMixLevels();
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscDelay)
};