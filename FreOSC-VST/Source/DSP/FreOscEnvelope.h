#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/**
    Custom ADSR Envelope with proper release behavior
    
    Unlike JUCE's ADSR, this envelope transitions smoothly to release 
    from any phase (attack/decay) without jumping to sustain level first.
*/
class FreOscEnvelope
{
public:
    //==============================================================================
    enum Phase
    {
        Idle = 0,
        Attack,
        Decay, 
        Sustain,
        Release
    };
    
    struct Parameters
    {
        float attack = 0.1f;   // seconds
        float decay = 0.3f;    // seconds  
        float sustain = 0.6f;  // level (0-1)
        float release = 0.5f;  // seconds
    };
    
    //==============================================================================
    FreOscEnvelope();
    ~FreOscEnvelope();
    
    //==============================================================================
    void setSampleRate(double sampleRate);
    void setParameters(const Parameters& params);
    
    //==============================================================================
    void noteOn();
    void noteOff();
    void reset();
    
    //==============================================================================
    float getNextSample();
    bool isActive() const;
    Phase getCurrentPhase() const { return currentPhase; }
    
private:
    //==============================================================================
    Parameters parameters;
    double sampleRate = 44100.0;
    
    Phase currentPhase = Idle;
    float currentLevel = 0.0f;
    float targetLevel = 0.0f;
    
    // Rate calculations (samples per second)
    float attackRate = 0.0f;
    float decayRate = 0.0f; 
    float releaseRate = 0.0f;
    
    //==============================================================================
    void calculateRates();
    void setPhase(Phase newPhase);
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscEnvelope)
};