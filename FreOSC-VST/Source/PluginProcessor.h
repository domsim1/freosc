#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/FreOscVoice.h"
#include "DSP/FreOscSound.h"
#include "DSP/FreOscFilter.h"
#include "DSP/FreOscPlateReverb.h"
#include "DSP/FreOscTapeDelay.h"
#include "DSP/FreOscWavefolder.h"
#include "DSP/FreOscLFO.h"
#include "Parameters/FreOscParameters.h"
#include "Presets/JsonPresetManager.h"

//==============================================================================
/**
    FreOSC VST Plugin Processor

    Main audio processor class that handles:
    - Polyphonic voice management
    - Parameter management
    - Effects processing chain
    - Preset system
    - MIDI input
*/
class FreOscProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    FreOscProcessor();
    ~FreOscProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Public interface for editor
    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }
    JsonPresetManager& getPresets() { return presets; }

    // Preset loading interface
    void loadPreset(int presetIndex);
    void loadPreset(const juce::String& presetName);

private:
    //==============================================================================
    // Parameter management
    juce::AudioProcessorValueTreeState parameters;

    // Voice management
    juce::Synthesiser synthesiser;

    // Effects chain using JUCE DSP (filter now per-voice)
    juce::dsp::ProcessorChain<
        juce::dsp::Compressor<float>,      // Compressor
        juce::dsp::Limiter<float>,         // Limiter
        FreOscPlateReverb,                 // Plate Reverb (custom)
        FreOscTapeDelay,                   // Tape Delay (custom)
        FreOscWavefolder                   // Wavefolder Distortion (custom)
    > effectsChain;

    // Preset management
    JsonPresetManager presets;

    // Audio processing state
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Global LFO for filter modulation
    FreOscLFO globalLFO;
    
    // Master volume smoothing to prevent pops
    juce::LinearSmoothedValue<float> masterVolumeSmooth;

    //==============================================================================
    // Parameter update methods
    void updateVoiceParameters();
    void updateEffectsParameters();

    // Helper methods
    void initializeSynthesiser();
    void setupEffectsChain();
    void processEffectsWithRouting(juce::dsp::ProcessContextReplacing<float>& context, int routingMode);
    
    // Master volume conversion: 0.0 = silence, 0.75 = 0dB (unity), 1.0 = +24dB
    float normalizedToMasterGain(float normalized) const;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreOscProcessor)
};

//==============================================================================
// Parameter IDs for consistent access across processor and editor
namespace ParameterIDs
{
    // Oscillator 1
    inline const juce::String osc1Waveform  = "osc1_waveform";
    inline const juce::String osc1Octave    = "osc1_octave";
    inline const juce::String osc1Level     = "osc1_level";
    inline const juce::String osc1Detune    = "osc1_detune";
    inline const juce::String osc1Pan       = "osc1_pan";

    // Oscillator 2
    inline const juce::String osc2Waveform  = "osc2_waveform";
    inline const juce::String osc2Octave    = "osc2_octave";
    inline const juce::String osc2Level     = "osc2_level";
    inline const juce::String osc2Detune    = "osc2_detune";
    inline const juce::String osc2Pan       = "osc2_pan";

    // Oscillator 3
    inline const juce::String osc3Waveform  = "osc3_waveform";
    inline const juce::String osc3Octave    = "osc3_octave";
    inline const juce::String osc3Level     = "osc3_level";
    inline const juce::String osc3Detune    = "osc3_detune";
    inline const juce::String osc3Pan       = "osc3_pan";

    // Noise
    inline const juce::String noiseType     = "noise_type";
    inline const juce::String noiseLevel    = "noise_level";
    inline const juce::String noisePan      = "noise_pan";

    // Master
    inline const juce::String masterVolume  = "master_volume";

    // Envelope
    inline const juce::String envelopeAttack   = "envelope_attack";
    inline const juce::String envelopeDecay    = "envelope_decay";
    inline const juce::String envelopeSustain  = "envelope_sustain";
    inline const juce::String envelopeRelease  = "envelope_release";

    // Filter
    inline const juce::String filterType       = "filter_type";
    inline const juce::String filterCutoff     = "filter_cutoff";
    inline const juce::String filterResonance  = "filter_resonance";
    inline const juce::String filterGain       = "filter_gain";

    // FM Synthesis
    inline const juce::String fmAmount         = "fm_amount";
    inline const juce::String fmSource         = "fm_source";
    inline const juce::String fmTarget         = "fm_target";
    inline const juce::String fmRatio          = "fm_ratio";

    // Dynamics removed - now uses fixed internal settings

    // Plate Reverb
    inline const juce::String platePreDelay    = "plate_predelay";
    inline const juce::String plateSize        = "plate_size";
    inline const juce::String plateDamping     = "plate_damping";
    inline const juce::String plateDiffusion   = "plate_diffusion";
    inline const juce::String plateWetLevel    = "plate_wet_level";
    inline const juce::String plateWidth       = "plate_width";

    // Tape Delay
    inline const juce::String tapeTime         = "tape_time";
    inline const juce::String tapeFeedback     = "tape_feedback";
    inline const juce::String tapeTone         = "tape_tone";
    inline const juce::String tapeFlutter      = "tape_flutter";
    inline const juce::String tapeWetLevel     = "tape_wet_level";
    inline const juce::String tapeWidth        = "tape_width";

    // LFO
    inline const juce::String lfoWaveform      = "lfo_waveform";
    inline const juce::String lfoRate          = "lfo_rate";
    inline const juce::String lfoTarget        = "lfo_target";
    inline const juce::String lfoAmount        = "lfo_amount";

    // Modulation Envelope 1
    inline const juce::String modEnv1Attack   = "mod_env1_attack";
    inline const juce::String modEnv1Decay    = "mod_env1_decay";
    inline const juce::String modEnv1Sustain  = "mod_env1_sustain";
    inline const juce::String modEnv1Release  = "mod_env1_release";
    inline const juce::String modEnv1Amount   = "mod_env1_amount";
    inline const juce::String modEnv1Target   = "mod_env1_target";

    // Modulation Envelope 2
    inline const juce::String modEnv2Attack   = "mod_env2_attack";
    inline const juce::String modEnv2Decay    = "mod_env2_decay";
    inline const juce::String modEnv2Sustain  = "mod_env2_sustain";
    inline const juce::String modEnv2Release  = "mod_env2_release";
    inline const juce::String modEnv2Amount   = "mod_env2_amount";
    inline const juce::String modEnv2Target   = "mod_env2_target";
}