#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

//==============================================================================
FreOscProcessor::FreOscProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
       parameters(*this, nullptr, "Parameters", FreOscParameters::createParameterLayout())
{
    // Initialize synthesizer
    initializeSynthesiser();

    // Set up effects chain
    setupEffectsChain();

    // Initialize preset manager with presets folder
    auto presetFolder = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                            .getParentDirectory()
                            .getChildFile("Presets");
    presets.initialize(presetFolder);
}

FreOscProcessor::~FreOscProcessor()
{
}

//==============================================================================
const juce::String FreOscProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FreOscProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FreOscProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FreOscProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FreOscProcessor::getTailLengthSeconds() const
{
    // Calculate tail length based on reverb and delay parameters
    double reverbTail = 0.0;
    double delayTail = 0.0;

    // Get reverb parameters
    auto reverbWetLevel = parameters.getRawParameterValue("reverb_wet_level")->load();
    auto reverbRoomSize = parameters.getRawParameterValue("reverb_room_size")->load();

    // Get delay parameters
    auto delayWetLevel = parameters.getRawParameterValue("delay_wet_level")->load();
    auto delayTime = parameters.getRawParameterValue("delay_time")->load();
    auto delayFeedback = parameters.getRawParameterValue("delay_feedback")->load();

    // Calculate reverb tail (if reverb is active)
    if (reverbWetLevel > 0.01f)
    {
        // Reverb tail is proportional to room size
        // Larger rooms = longer decay times
        reverbTail = reverbRoomSize * 4.0; // Up to 4 seconds for maximum room size
    }

    // Calculate delay tail (if delay is active)
    if (delayWetLevel > 0.01f && delayFeedback > 0.01f)
    {
        // Calculate decay time based on feedback amount
        // Higher feedback = longer tail
        double decayTime = -std::log(0.001) / std::log(delayFeedback);
        delayTail = delayTime / 1000.0 * decayTime; // Convert ms to seconds and multiply by decay factor
    }

    // Return the maximum of reverb and delay tails
    return std::max(reverbTail, delayTail);
}

int FreOscProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FreOscProcessor::getCurrentProgram()
{
    return 0;
}

void FreOscProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String FreOscProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void FreOscProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void FreOscProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    // Prepare synthesizer
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);

    // Prepare effects chain
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2; // Stereo

    effectsChain.prepare(spec);

    // Prepare global LFO for filter modulation
    globalLFO.prepare(sampleRate);
    
    // Initialize master volume smoothing
    masterVolumeSmooth.reset(sampleRate, 0.05); // 50ms ramp time
    float initialMasterVol = parameters.getRawParameterValue(ParameterIDs::masterVolume)->load();
    masterVolumeSmooth.setCurrentAndTargetValue(initialMasterVol);

    // Update all voice parameters
    updateVoiceParameters();
}

void FreOscProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    effectsChain.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FreOscProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FreOscProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update parameters from UI
    updateVoiceParameters();
    updateEffectsParameters();

    // Clear the buffer first (synthesizer will add to it)
    buffer.clear();

    // Remove the forced silence check - let synthesizer handle voice management naturally

    // Render synthesizer (filtering now happens per-voice)
    synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Process through global effects chain (compressor → limiter → reverb → delay)
    // Filter processing now happens inside each voice
    auto audioBlock = juce::dsp::AudioBlock<float>(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    effectsChain.process(context);

    // Apply smoothed master volume to prevent pops
    float targetMasterVol = parameters.getRawParameterValue(ParameterIDs::masterVolume)->load();
    masterVolumeSmooth.setTargetValue(targetMasterVol);
    
    // Apply gain sample by sample for smooth transitions
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float smoothedGain = masterVolumeSmooth.getCurrentValue();
            channelData[sample] *= smoothedGain;
            
            // Advance smoothing only once per sample (not per channel)
            if (channel == 0)
                masterVolumeSmooth.getNextValue();
        }
    }
}

//==============================================================================
bool FreOscProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FreOscProcessor::createEditor()
{
    // Return our custom FreOSC editor with organized sections
    return new FreOscEditor(*this);
}

//==============================================================================
void FreOscProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save plugin state
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FreOscProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore plugin state
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
void FreOscProcessor::initializeSynthesiser()
{
    // Add voices (16 voice polyphony)
    for (int i = 0; i < 16; ++i)
    {
        synthesiser.addVoice(new FreOscVoice());
    }

    // Add sound - FreOSC uses one sound type for all notes
    synthesiser.addSound(new FreOscSound());
}

void FreOscProcessor::setupEffectsChain()
{
    // Initialize effects chain components with fixed settings optimized for polyphonic synthesis
    // Order: Compressor -> Limiter -> Reverb -> Delay (filter now per-voice)

    // Set up compressor with very gentle settings for polyphonic chords
    auto& compressor = effectsChain.get<0>();
    compressor.setThreshold(-60.0f);  // Extremely high threshold - essentially disabled
    compressor.setRatio(1.1f);        // Barely any compression
    compressor.setAttack(0.1f);       // Slow attack
    compressor.setRelease(1.0f);      // Slow release

    // Set up limiter with very conservative settings
    auto& limiter = effectsChain.get<1>();
    limiter.setThreshold(-6.0f);      // Very conservative threshold
    limiter.setRelease(0.1f);         // Slow release to prevent artifacts
}

void FreOscProcessor::updateVoiceParameters()
{
    // Get current parameter values
    auto osc1Waveform = static_cast<int>(parameters.getRawParameterValue("osc1_waveform")->load());
    auto osc1Octave = static_cast<int>(parameters.getRawParameterValue("osc1_octave")->load());
    auto osc1Level = parameters.getRawParameterValue("osc1_level")->load();
    auto osc1Detune = parameters.getRawParameterValue("osc1_detune")->load();
    auto osc1Pan = parameters.getRawParameterValue("osc1_pan")->load();

    auto osc2Waveform = static_cast<int>(parameters.getRawParameterValue("osc2_waveform")->load());
    auto osc2Octave = static_cast<int>(parameters.getRawParameterValue("osc2_octave")->load());
    auto osc2Level = parameters.getRawParameterValue("osc2_level")->load();
    auto osc2Detune = parameters.getRawParameterValue("osc2_detune")->load();
    auto osc2Pan = parameters.getRawParameterValue("osc2_pan")->load();

    auto osc3Waveform = static_cast<int>(parameters.getRawParameterValue("osc3_waveform")->load());
    auto osc3Octave = static_cast<int>(parameters.getRawParameterValue("osc3_octave")->load());
    auto osc3Level = parameters.getRawParameterValue("osc3_level")->load();
    auto osc3Detune = parameters.getRawParameterValue("osc3_detune")->load();
    auto osc3Pan = parameters.getRawParameterValue("osc3_pan")->load();

    auto noiseType = static_cast<int>(parameters.getRawParameterValue("noise_type")->load());
    auto noiseLevel = parameters.getRawParameterValue("noise_level")->load();
    auto noisePan = parameters.getRawParameterValue("noise_pan")->load();

    auto attack = parameters.getRawParameterValue("envelope_attack")->load();
    auto decay = parameters.getRawParameterValue("envelope_decay")->load();
    auto sustain = parameters.getRawParameterValue("envelope_sustain")->load();
    auto release = parameters.getRawParameterValue("envelope_release")->load();

    auto fmAmount = parameters.getRawParameterValue("fm_amount")->load();
    auto fmSource = static_cast<int>(parameters.getRawParameterValue("fm_source")->load());
    auto fmTarget = static_cast<int>(parameters.getRawParameterValue("fm_target")->load());
    auto fmRatio = parameters.getRawParameterValue("fm_ratio")->load();

    auto lfoWaveform = static_cast<int>(parameters.getRawParameterValue("lfo_waveform")->load());
    auto lfoRate = parameters.getRawParameterValue("lfo_rate")->load();
    auto lfoTarget = static_cast<int>(parameters.getRawParameterValue("lfo_target")->load());
    auto lfoAmount = parameters.getRawParameterValue("lfo_amount")->load();

    auto filterType = static_cast<int>(parameters.getRawParameterValue("filter_type")->load());
    auto filterCutoff = parameters.getRawParameterValue("filter_cutoff")->load();
    auto filterResonance = parameters.getRawParameterValue("filter_resonance")->load();
    auto filterGain = parameters.getRawParameterValue("filter_gain")->load();
    auto formantVowel = static_cast<int>(parameters.getRawParameterValue("formant_vowel")->load());

    // Update all voices with current parameters
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<FreOscVoice*>(synthesiser.getVoice(i)))
        {
            voice->updateOscillatorParameters(
                osc1Waveform, osc1Octave, osc1Level, osc1Detune, osc1Pan,
                osc2Waveform, osc2Octave, osc2Level, osc2Detune, osc2Pan,
                osc3Waveform, osc3Octave, osc3Level, osc3Detune, osc3Pan
            );

            voice->updateNoiseParameters(noiseType, noiseLevel, noisePan);
            voice->updateEnvelopeParameters(attack, decay, sustain, release);
            voice->updateFMParameters(fmAmount, fmSource, fmTarget, fmRatio);
            voice->updateLFOParameters(lfoWaveform, lfoRate, lfoTarget, lfoAmount);
            voice->updateFilterParameters(filterType, filterCutoff, filterResonance, filterGain, formantVowel);
        }
    }
}

void FreOscProcessor::updateEffectsParameters()
{
    // Dynamics (compressor/limiter) now use fixed settings - no user control
    // This prevents distortion issues with polyphonic synthesis

    // Update reverb parameters (now at index 2)
    auto& reverb = effectsChain.get<2>();
    reverb.setRoomSize(parameters.getRawParameterValue("reverb_room_size")->load());
    reverb.setWetLevel(parameters.getRawParameterValue("reverb_wet_level")->load());

    // Update delay parameters (now at index 3)
    auto& delay = effectsChain.get<3>();
    delay.setDelayTime(parameters.getRawParameterValue("delay_time")->load());
    delay.setFeedback(parameters.getRawParameterValue("delay_feedback")->load());
    delay.setWetLevel(parameters.getRawParameterValue("delay_wet_level")->load());
}

//==============================================================================
void FreOscProcessor::loadPreset(int presetIndex)
{
    presets.loadPreset(presetIndex, parameters);
}

void FreOscProcessor::loadPreset(const juce::String& presetName)
{
    presets.loadPreset(presetName, parameters);
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FreOscProcessor();
}