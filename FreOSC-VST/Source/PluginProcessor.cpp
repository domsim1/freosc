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

    // Get plate reverb parameters
    auto reverbWetLevel = parameters.getRawParameterValue("plate_wet_level")->load();
    auto reverbRoomSize = parameters.getRawParameterValue("plate_size")->load();

    // Get tape delay parameters
    auto delayWetLevel = parameters.getRawParameterValue("tape_wet_level")->load();
    auto delayTime = parameters.getRawParameterValue("tape_time")->load() * (2000.0f - 20.0f) + 20.0f; // Convert normalized to ms
    auto delayFeedback = parameters.getRawParameterValue("tape_feedback")->load();

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
    float initialMasterVolNormalized = parameters.getRawParameterValue(ParameterIDs::masterVolume)->load();
    float initialMasterVol = normalizedToMasterGain(initialMasterVolNormalized);
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

    // Process through global effects chain with routing support
    // Filter processing now happens inside each voice
    auto audioBlock = juce::dsp::AudioBlock<float>(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    
    // Always apply clean compressor and limiter first
    auto& compressor = effectsChain.get<0>();
    auto& limiter = effectsChain.get<1>();
    compressor.process(context);
    limiter.process(context);
    
    // Apply effects routing based on parameter
    auto effectsRouting = static_cast<int>(parameters.getRawParameterValue("effects_routing")->load());
    processEffectsWithRouting(context, effectsRouting);

    // Apply smoothed master volume to prevent pops
    float targetMasterVolNormalized = parameters.getRawParameterValue(ParameterIDs::masterVolume)->load();
    float targetMasterVol = normalizedToMasterGain(targetMasterVolNormalized);
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
    // Initialize effects chain components with musical settings
    // Order: Clean Compressor -> Clean Limiter -> Reverb -> Delay (filter now per-voice)

    // Set up clean compressor with musical defaults
    auto& compressor = effectsChain.get<0>();
    compressor.setThreshold(-12.0f);   // Musical threshold
    compressor.setRatio(4.0f);         // Musical compression ratio
    compressor.setAttack(1.0f);        // Fast attack
    compressor.setRelease(100.0f);     // Musical release
    compressor.setKnee(2.0f);          // Soft knee
    compressor.setMakeupGain(0.0f);    // No initial makeup gain
    compressor.setMix(1.0f);           // Full wet

    // Set up clean limiter with musical defaults
    auto& limiter = effectsChain.get<1>();
    limiter.setThreshold(-3.0f);       // Musical threshold
    limiter.setRelease(50.0f);         // Musical release
    limiter.setCeiling(-0.1f);         // Safe ceiling
    limiter.setSaturation(0.3f);       // Musical saturation
    limiter.setLookahead(2.0f);        // 2ms lookahead
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

    auto pmIndex = parameters.getRawParameterValue("pm_index")->load();
    auto pmCarrier = static_cast<int>(parameters.getRawParameterValue("pm_carrier")->load());
    auto pmRatio = parameters.getRawParameterValue("pm_ratio")->load();

    auto lfoWaveform = static_cast<int>(parameters.getRawParameterValue("lfo_waveform")->load());
    auto lfoRate = parameters.getRawParameterValue("lfo_rate")->load();
    auto lfoTarget = static_cast<int>(parameters.getRawParameterValue("lfo_target")->load());
    auto lfoAmount = parameters.getRawParameterValue("lfo_amount")->load();

    auto lfo2Waveform = static_cast<int>(parameters.getRawParameterValue("lfo2_waveform")->load());
    auto lfo2Rate = parameters.getRawParameterValue("lfo2_rate")->load();
    auto lfo2Target = static_cast<int>(parameters.getRawParameterValue("lfo2_target")->load());
    auto lfo2Amount = parameters.getRawParameterValue("lfo2_amount")->load();

    auto lfo3Waveform = static_cast<int>(parameters.getRawParameterValue("lfo3_waveform")->load());
    auto lfo3Rate = parameters.getRawParameterValue("lfo3_rate")->load();
    auto lfo3Target = static_cast<int>(parameters.getRawParameterValue("lfo3_target")->load());
    auto lfo3Amount = parameters.getRawParameterValue("lfo3_amount")->load();

    auto filterType = static_cast<int>(parameters.getRawParameterValue("filter_type")->load());
    auto filterCutoff = parameters.getRawParameterValue("filter_cutoff")->load();
    auto filterResonance = parameters.getRawParameterValue("filter_resonance")->load();
    auto filterGain = parameters.getRawParameterValue("filter_gain")->load();

    auto filter2Type = static_cast<int>(parameters.getRawParameterValue("filter2_type")->load());
    auto filter2Cutoff = parameters.getRawParameterValue("filter2_cutoff")->load();
    auto filter2Resonance = parameters.getRawParameterValue("filter2_resonance")->load();
    auto filter2Gain = parameters.getRawParameterValue("filter2_gain")->load();
    auto filterRouting = static_cast<int>(parameters.getRawParameterValue("filter_routing")->load());

    // Modulation Envelope 1 parameters
    auto modEnv1Attack = parameters.getRawParameterValue("mod_env1_attack")->load();
    auto modEnv1Decay = parameters.getRawParameterValue("mod_env1_decay")->load();
    auto modEnv1Sustain = parameters.getRawParameterValue("mod_env1_sustain")->load();
    auto modEnv1Release = parameters.getRawParameterValue("mod_env1_release")->load();
    auto modEnv1Amount = parameters.getRawParameterValue("mod_env1_amount")->load();
    auto modEnv1Target = static_cast<int>(parameters.getRawParameterValue("mod_env1_target")->load());
    auto modEnv1Mode = static_cast<int>(parameters.getRawParameterValue("mod_env1_mode")->load());
    auto modEnv1Rate = parameters.getRawParameterValue("mod_env1_rate")->load();

    // Modulation Envelope 2 parameters
    auto modEnv2Attack = parameters.getRawParameterValue("mod_env2_attack")->load();
    auto modEnv2Decay = parameters.getRawParameterValue("mod_env2_decay")->load();
    auto modEnv2Sustain = parameters.getRawParameterValue("mod_env2_sustain")->load();
    auto modEnv2Release = parameters.getRawParameterValue("mod_env2_release")->load();
    auto modEnv2Amount = parameters.getRawParameterValue("mod_env2_amount")->load();
    auto modEnv2Target = static_cast<int>(parameters.getRawParameterValue("mod_env2_target")->load());
    auto modEnv2Mode = static_cast<int>(parameters.getRawParameterValue("mod_env2_mode")->load());
    auto modEnv2Rate = parameters.getRawParameterValue("mod_env2_rate")->load();

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
            voice->updatePMParameters(pmIndex, pmCarrier, pmRatio);
            voice->updateLFOParameters(lfoWaveform, lfoRate, lfoTarget, lfoAmount);
            voice->updateLFO2Parameters(lfo2Waveform, lfo2Rate, lfo2Target, lfo2Amount);
            voice->updateLFO3Parameters(lfo3Waveform, lfo3Rate, lfo3Target, lfo3Amount);
            voice->updateFilterParameters(filterType, filterCutoff, filterResonance, filterGain);
            voice->updateFilter2Parameters(filter2Type, filter2Cutoff, filter2Resonance, filter2Gain);
            voice->updateFilterRouting(filterRouting);
            voice->updateModEnv1Parameters(modEnv1Attack, modEnv1Decay, modEnv1Sustain, modEnv1Release, modEnv1Amount, modEnv1Target, modEnv1Mode, modEnv1Rate);
            voice->updateModEnv2Parameters(modEnv2Attack, modEnv2Decay, modEnv2Sustain, modEnv2Release, modEnv2Amount, modEnv2Target, modEnv2Mode, modEnv2Rate);
        }
    }
}

void FreOscProcessor::updateEffectsParameters()
{
    // Update clean compressor parameters (index 0)
    auto& compressor = effectsChain.get<0>();
    compressor.setThreshold(parameters.getRawParameterValue("comp_threshold")->load());
    compressor.setRatio(parameters.getRawParameterValue("comp_ratio")->load());
    compressor.setAttack(parameters.getRawParameterValue("comp_attack")->load());
    compressor.setRelease(parameters.getRawParameterValue("comp_release")->load());
    compressor.setMakeupGain(parameters.getRawParameterValue("comp_makeup")->load());
    compressor.setMix(parameters.getRawParameterValue("comp_mix")->load());
    
    // Update clean limiter parameters (index 1)
    auto& limiter = effectsChain.get<1>();
    limiter.setThreshold(parameters.getRawParameterValue("limiter_threshold")->load());
    limiter.setRelease(parameters.getRawParameterValue("limiter_release")->load());
    limiter.setCeiling(parameters.getRawParameterValue("limiter_ceiling")->load());
    limiter.setSaturation(parameters.getRawParameterValue("limiter_saturation")->load());

    // Update plate reverb parameters (now at index 2)
    auto& plateReverb = effectsChain.get<2>();
    plateReverb.setPreDelay(parameters.getRawParameterValue("plate_predelay")->load());
    plateReverb.setSize(parameters.getRawParameterValue("plate_size")->load());
    plateReverb.setDamping(parameters.getRawParameterValue("plate_damping")->load());
    plateReverb.setDiffusion(parameters.getRawParameterValue("plate_diffusion")->load());
    plateReverb.setWetLevel(parameters.getRawParameterValue("plate_wet_level")->load());
    plateReverb.setStereoWidth(parameters.getRawParameterValue("plate_width")->load());

    // Update tape delay parameters (now at index 3)
    auto& tapeDelay = effectsChain.get<3>();
    tapeDelay.setTime(parameters.getRawParameterValue("tape_time")->load());
    tapeDelay.setFeedback(parameters.getRawParameterValue("tape_feedback")->load());
    tapeDelay.setTone(parameters.getRawParameterValue("tape_tone")->load());
    tapeDelay.setFlutter(parameters.getRawParameterValue("tape_flutter")->load());
    tapeDelay.setWetLevel(parameters.getRawParameterValue("tape_wet_level")->load());
    tapeDelay.setStereoWidth(parameters.getRawParameterValue("tape_width")->load());

    // Update wavefolder parameters (now at index 4)
    auto& wavefolder = effectsChain.get<4>();
    wavefolder.setDrive(parameters.getRawParameterValue("wavefolder_drive")->load());
    wavefolder.setThreshold(parameters.getRawParameterValue("wavefolder_threshold")->load());
    wavefolder.setSymmetry(parameters.getRawParameterValue("wavefolder_symmetry")->load());
    wavefolder.setMix(parameters.getRawParameterValue("wavefolder_mix")->load());
    wavefolder.setOutputLevel(parameters.getRawParameterValue("wavefolder_output")->load());
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
// Preset saving/updating implementation

bool FreOscProcessor::saveUserPreset(const juce::String& name, const juce::String& description)
{
    return presets.saveUserPreset(name, description, parameters);
}

bool FreOscProcessor::updatePreset(int presetIndex)
{
    return presets.updatePreset(presetIndex, parameters);
}

bool FreOscProcessor::updatePreset(const juce::String& presetName)
{
    return presets.updatePreset(presetName, parameters);
}

bool FreOscProcessor::deletePreset(int presetIndex)
{
    return presets.deletePreset(presetIndex);
}

bool FreOscProcessor::deletePreset(const juce::String& presetName)
{
    return presets.deletePreset(presetName);
}

bool FreOscProcessor::presetExists(const juce::String& presetName)
{
    return presets.presetExists(presetName);
}

juce::String FreOscProcessor::getCurrentPresetName()
{
    return presets.getCurrentPresetName();
}

void FreOscProcessor::clearCurrentPreset()
{
    presets.clearCurrentPreset();
}

//==============================================================================
// Master volume conversion function
float FreOscProcessor::normalizedToMasterGain(float normalized) const
{
    // Clamp input to valid range
    normalized = juce::jlimit(0.0f, 1.0f, normalized);
    
    // Handle silence (0.0 = complete silence)
    if (normalized <= 0.0f)
        return 0.0f;
    
    // Map 0.75 to 0dB (unity gain), with smooth curve
    // 0.0 = silence, 0.75 = 0dB, 1.0 = +24dB
    
    if (normalized <= 0.75f)
    {
        // Map 0.0-0.75 to -∞dB to 0dB (logarithmic curve for attenuation)
        float normalizedAttenuation = normalized / 0.75f; // 0.0 to 1.0
        float dbValue = -60.0f + (normalizedAttenuation * 60.0f); // -60dB to 0dB
        return juce::Decibels::decibelsToGain(dbValue);
    }
    else
    {
        // Map 0.75-1.0 to 0dB to +24dB (linear in dB space for boost)
        float normalizedBoost = (normalized - 0.75f) / 0.25f; // 0.0 to 1.0
        float dbValue = normalizedBoost * 24.0f; // 0dB to +24dB
        return juce::Decibels::decibelsToGain(dbValue);
    }
}

//==============================================================================
void FreOscProcessor::processEffectsWithRouting(juce::dsp::ProcessContextReplacing<float>& context, int routingMode)
{
    auto& plateReverb = effectsChain.get<2>();
    auto& tapeDelay = effectsChain.get<3>();
    auto& wavefolder = effectsChain.get<4>();
    
    switch (routingMode)
    {
        case 0: // Wavefolder to Reverb to Delay
        {
            wavefolder.process(context);
            plateReverb.process(context);
            tapeDelay.process(context);
            break;
        }
        
        case 1: // Wavefolder to Delay to Reverb
        {
            wavefolder.process(context);
            tapeDelay.process(context);
            plateReverb.process(context);
            break;
        }
        
        case 2: // Wavefolder Parallel with Reverb+Delay
        {
            // Create buffers for parallel processing
            auto& inputBlock = context.getInputBlock();
            
            juce::AudioBuffer<float> wavefolderBuffer(static_cast<int>(inputBlock.getNumChannels()), 
                                                      static_cast<int>(inputBlock.getNumSamples()));
            juce::AudioBuffer<float> reverbDelayBuffer(static_cast<int>(inputBlock.getNumChannels()), 
                                                       static_cast<int>(inputBlock.getNumSamples()));
            
            // Copy input to both buffers
            for (int ch = 0; ch < static_cast<int>(inputBlock.getNumChannels()); ++ch)
            {
                for (int sample = 0; sample < static_cast<int>(inputBlock.getNumSamples()); ++sample)
                {
                    auto inputSample = inputBlock.getSample(ch, sample);
                    wavefolderBuffer.setSample(ch, sample, inputSample);
                    reverbDelayBuffer.setSample(ch, sample, inputSample);
                }
            }
            
            // Process wavefolder on one path
            auto wavefolderBlock = juce::dsp::AudioBlock<float>(wavefolderBuffer);
            juce::dsp::ProcessContextReplacing<float> wavefolderContext(wavefolderBlock);
            wavefolder.process(wavefolderContext);
            
            // Process reverb+delay on the other path
            auto reverbDelayBlock = juce::dsp::AudioBlock<float>(reverbDelayBuffer);
            juce::dsp::ProcessContextReplacing<float> reverbDelayContext(reverbDelayBlock);
            plateReverb.process(reverbDelayContext);
            tapeDelay.process(reverbDelayContext);
            
            // Mix the parallel outputs
            auto& outputBlock = context.getOutputBlock();
            for (int ch = 0; ch < static_cast<int>(outputBlock.getNumChannels()); ++ch)
            {
                for (int sample = 0; sample < static_cast<int>(outputBlock.getNumSamples()); ++sample)
                {
                    auto wavefolderSample = wavefolderBuffer.getSample(ch, sample);
                    auto reverbDelaySample = reverbDelayBuffer.getSample(ch, sample);
                    
                    // Mix the parallel paths
                    outputBlock.setSample(ch, sample, (wavefolderSample + reverbDelaySample) * 0.5f);
                }
            }
            break;
        }
        
        default:
            // Fallback to first wavefolder routing
            wavefolder.process(context);
            plateReverb.process(context);
            tapeDelay.process(context);
            break;
    }
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FreOscProcessor();
}