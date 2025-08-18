#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

/**
 * FreOscTapeDelay - Analog tape delay simulation
 * 
 * Features:
 * - Warm analog tape delay characteristics
 * - Time control (20ms - 2000ms)
 * - Feedback with tape saturation
 * - Tape wear/flutter simulation
 * - High-frequency rolloff (tape aging)
 * - Wet/dry mixing
 * - Stereo width control for spacious delays
 */
class FreOscTapeDelay
{
public:
    //==============================================================================
    FreOscTapeDelay();
    ~FreOscTapeDelay();

    //==============================================================================
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(const juce::dsp::ProcessContextReplacing<float>& context);

    //==============================================================================
    // Parameter setters (0.0 to 1.0 normalized range)
    void setTime(float time);              // 20ms to 2000ms
    void setFeedback(float feedback);      // Feedback amount with tape saturation
    void setTone(float tone);              // High-frequency rolloff (tape aging)
    void setFlutter(float flutter);        // Tape flutter/wow amount
    void setWetLevel(float wetLevel);      // Wet signal level
    void setStereoWidth(float width);      // Stereo spread (0=mono, 1=full stereo)

private:
    //==============================================================================
    // Core tape delay algorithm
    struct TapeDelayLine
    {
        std::vector<float> buffer;
        int writePos = 0;
        int size = 0;
        
        void setSize(int newSize);
        void clear();
        float read(float delaySamples) const;
        void write(float sample);
        float readInterpolated(float delaySamples) const;
    };
    
    //==============================================================================
    // Tape delay network structure
    TapeDelayLine delayLineL, delayLineR;
    
    // Tape characteristics filters
    juce::dsp::IIR::Filter<float> tapeFilterL, tapeFilterR;
    juce::dsp::IIR::Filter<float> feedbackFilterL, feedbackFilterR;
    
    // Tape saturation/compression
    juce::dsp::Compressor<float> tapeSaturation;
    
    // Simple manual flutter oscillator (thread-safe)
    float flutterPhase = 0.0f;
    
    // Gain calculation done directly in process method for thread safety
    
    //==============================================================================
    // Parameters
    double sampleRate = 44100.0;
    
    float currentTime = 0.25f;         // 0.0 to 1.0 (20ms to 2000ms)
    float currentFeedback = 0.3f;      // 0.0 to 1.0
    float currentTone = 0.7f;          // 0.0 to 1.0 (darker to brighter)
    float currentFlutter = 0.1f;       // 0.0 to 1.0 (subtle tape imperfections)
    float currentWetLevel = 0.2f;      // 0.0 to 1.0
    float currentStereoWidth = 0.6f;   // 0.0 to 1.0
    
    // Flutter control
    float flutterDepth = 0.0f;
    float flutterRate = 0.3f;          // Hz
    float flutterPhaseIncrement = 0.0f;
    
    //==============================================================================
    // Helper methods
    void updateDelayTimes();
    void updateTapeFilters();
    void updateFlutter();
    void updateStereoWidth();
    
    // Tape saturation simulation
    float applySaturation(float input, float drive);
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscTapeDelay)
};