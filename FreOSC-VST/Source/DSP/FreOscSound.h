#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
/**
    FreOSC Sound Class
    
    Simple sound class that allows all voices to play any sound.
    This is required by JUCE's Synthesiser class architecture.
    
    In FreOSC, we use a single sound type since the synthesizer
    is entirely parameter-based rather than sample-based.
*/
class FreOscSound : public juce::SynthesiserSound
{
public:
    //==============================================================================
    FreOscSound() {}
    ~FreOscSound() override {}
    
    //==============================================================================
    // SynthesiserSound interface
    bool appliesToNote(int midiNoteNumber) override 
    { 
        juce::ignoreUnused(midiNoteNumber);
        return true; // All notes are valid for this synthesizer
    }
    
    bool appliesToChannel(int midiChannel) override 
    { 
        juce::ignoreUnused(midiChannel);
        return true; // All MIDI channels are valid
    }
    
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscSound)
};