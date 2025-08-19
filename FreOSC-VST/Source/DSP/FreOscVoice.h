#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "FreOscOscillator.h"
#include "FreOscNoiseGenerator.h"
#include "FreOscLFO.h"
#include "FreOscSound.h"
#include "FreOscFilter.h"

//==============================================================================
/**
    FreOSC Synthesizer Voice

    Represents a single voice in the polyphonic synthesizer.
    Each voice contains:
    - 3 oscillators with independent control
    - Noise generator
    - ADSR envelope
    - LFO modulation
    - PM (Phase Modulation) synthesis capabilities
*/
class FreOscVoice : public juce::SynthesiserVoice
{
public:
    //==============================================================================
    // Filter routing modes
    enum FilterRouting
    {
        FilterOff = 0,      // Only Filter 1 active
        FilterParallel,     // Both filters in parallel
        FilterSeries        // Filter 1 -> Filter 2 in series
    };
    //==============================================================================
    FreOscVoice();
    ~FreOscVoice() override;

    //==============================================================================
    // SynthesiserVoice overrides
    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    //==============================================================================
    // Voice state management
    bool isVoiceActive() const override;
    void setCurrentPlaybackSampleRate(double sampleRate) override;

    //==============================================================================
    // Parameter updates (called from processor)
    void updateOscillatorParameters(
        // Oscillator 1
        int osc1Waveform, int osc1Octave, float osc1Level, float osc1Detune, float osc1Pan,
        // Oscillator 2
        int osc2Waveform, int osc2Octave, float osc2Level, float osc2Detune, float osc2Pan,
        // Oscillator 3
        int osc3Waveform, int osc3Octave, float osc3Level, float osc3Detune, float osc3Pan
    );

    void updateNoiseParameters(int noiseType, float noiseLevel, float noisePan);

    void updateEnvelopeParameters(float attack, float decay, float sustain, float release);

    void updatePMParameters(float pmIndex, int pmCarrier, float pmRatio);

    void updateLFOParameters(int lfoWaveform, float lfoRate, int lfoTarget, float lfoAmount);
    void updateLFO2Parameters(int lfo2Waveform, float lfo2Rate, int lfo2Target, float lfo2Amount);
    void updateLFO3Parameters(int lfo3Waveform, float lfo3Rate, int lfo3Target, float lfo3Amount);

    void updateFilterParameters(int filterType, float cutoff, float resonance, float gain);
    
    void updateFilter2Parameters(int filter2Type, float cutoff2, float resonance2, float gain2);
    
    void updateFilterRouting(int routing);

    void updateModEnv1Parameters(float attack, float decay, float sustain, float release, float amount, int target, int mode, float rate);

    void updateModEnv2Parameters(float attack, float decay, float sustain, float release, float amount, int target, int mode, float rate);

private:
    //==============================================================================
    // Audio components
    FreOscOscillator oscillator1, oscillator2, oscillator3;
    FreOscOscillator pmModulator; // Dedicated PM modulator (copies OSC3 settings)
    FreOscNoiseGenerator noiseGenerator;
    FreOscLFO lfo, lfo2, lfo3;

    // Envelope
    juce::ADSR envelope;
    juce::ADSR::Parameters envelopeParameters;

    // Modulation envelopes
    juce::ADSR modEnvelope1, modEnvelope2;
    juce::ADSR::Parameters modEnv1Parameters, modEnv2Parameters;
    
    // Envelope mode state tracking (atomic for thread safety)
    std::atomic<bool> modEnv1IsLooping{false}, modEnv2IsLooping{false};
    std::atomic<bool> modEnv1OneShotCompleted{false}, modEnv2OneShotCompleted{false};
    
    // Additional state for proper looping implementation
    std::atomic<int> modEnv1LastMode{1}, modEnv2LastMode{1}; // Track mode changes
    std::atomic<bool> modEnv1LoopingActive{false}, modEnv2LoopingActive{false};
    
    // Per-voice rate-based timing state
    double modEnv1CycleSamples = 0.0, modEnv2CycleSamples = 0.0;
    int modEnv1CurrentCycleSample = 0, modEnv2CurrentCycleSample = 0;
    bool modEnv1CycleActive = false, modEnv2CycleActive = false;

    // Panning (stereo positioning)
    juce::dsp::Panner<float> panner1, panner2, panner3, noisePanner;

    // PM synthesis (uses Oscillator 3 actual output as modulator)

    // Per-voice filters
    FreOscFilter voiceFilter;
    FreOscFilter voiceFilter2;

    //==============================================================================
    // Voice state
    double currentSampleRate = 44100.0;
    float currentNoteFrequency = 440.0f;
    int currentMidiNote = 60;
    float currentVelocity = 1.0f;
    bool noteIsOn = false;
    
    // Anti-pop ramping
    juce::LinearSmoothedValue<float> amplitudeRamp;
    bool isRampingDown = false;
    
    // DC blocking filter to prevent DC offset pops
    juce::dsp::IIR::Filter<float> dcBlocker;

    // MIDI modulation state
    float currentPitchBend = 0.0f;        // -1.0 to +1.0 (normalized)
    float pitchBendRange = 2.0f;          // semitones (+/- range)

    // CC modulation values (0.0 to 1.0, normalized)
    float ccModWheel = 0.0f;              // CC1: Modulation wheel
    float ccVolume = 1.0f;                // CC7: Volume
    float ccExpression = 1.0f;            // CC11: Expression
    float ccFilterCutoff = 0.0f;          // CC74: Filter cutoff
    float ccFilterResonance = 0.0f;       // CC71: Filter resonance

    // Current parameter values (thread-safe atomic where needed)
    struct VoiceParameters
    {
        // Oscillator levels and settings - very conservative for clean polyphony
        std::atomic<float> osc1Level{0.3f}, osc2Level{0.15f}, osc3Level{0.05f};
        std::atomic<float> osc1Pan{0.0f}, osc2Pan{-0.2f}, osc3Pan{0.2f};
        std::atomic<float> noiseLevel{0.0f}, noisePan{0.0f};

        // PM parameters
        std::atomic<float> pmIndex{0.0f}, pmRatio{1.0f};
        std::atomic<int> pmCarrier{0}; // 0=osc1, 1=osc2, 2=both

        // LFO parameters
        std::atomic<float> lfoRate{2.0f}, lfoAmount{0.0f};
        std::atomic<int> lfoWaveform{0}, lfoTarget{0}; // 0=sine/none
        
        // LFO 2 parameters
        std::atomic<float> lfo2Rate{2.0f}, lfo2Amount{0.0f};
        std::atomic<int> lfo2Waveform{0}, lfo2Target{0}; // 0=sine/none
        
        // LFO 3 parameters
        std::atomic<float> lfo3Rate{2.0f}, lfo3Amount{0.0f};
        std::atomic<int> lfo3Waveform{0}, lfo3Target{0}; // 0=sine/none

        // Filter parameters
        std::atomic<float> filterCutoff{0.5f}, filterResonance{0.1f}, filterGain{0.5f};
        std::atomic<int> filterType{0}; // 0=lowpass
        
        // Filter 2 parameters
        std::atomic<float> filter2Cutoff{0.5f}, filter2Resonance{0.1f}, filter2Gain{0.5f};
        std::atomic<int> filter2Type{0}; // 0=lowpass
        
        // Filter routing
        std::atomic<int> filterRouting{0}; // 0=off

        // Modulation Envelope 1 parameters
        std::atomic<float> modEnv1Amount{0.0f};
        std::atomic<int> modEnv1Target{0}; // 0=none
        std::atomic<int> modEnv1Mode{1}; // 0=One-Shot, 1=Gate, 2=Looping
        std::atomic<float> modEnv1Rate{1.0f}; // Hz for One-Shot/Looping modes

        // Modulation Envelope 2 parameters
        std::atomic<float> modEnv2Amount{0.0f};
        std::atomic<int> modEnv2Target{0}; // 0=none
        std::atomic<int> modEnv2Mode{1}; // 0=One-Shot, 1=Gate, 2=Looping
        std::atomic<float> modEnv2Rate{1.0f}; // Hz for One-Shot/Looping modes
    } params;

    //==============================================================================
    // Helper methods
    void setupOscillators();
    void calculateNoteFrequency(int midiNote, int octaveOffset, float detuneAmount);
    void syncPMModulatorWithOSC3(); // Copy OSC3 settings to PM modulator
    float getPMModulationSignal();
    bool shouldReceivePM(int oscillatorIndex);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreOscVoice)
};