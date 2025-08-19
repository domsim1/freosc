#include "FreOscVoice.h"
#include "FreOscSound.h"

//==============================================================================
FreOscVoice::FreOscVoice()
{
    // Initialize ADSR with default parameters
    envelopeParameters.attack = 0.1f;
    envelopeParameters.decay = 0.3f;
    envelopeParameters.sustain = 0.6f;
    envelopeParameters.release = 0.5f;
    envelope.setParameters(envelopeParameters);

    // Initialize modulation envelopes with default parameters
    modEnv1Parameters.attack = 0.01f;
    modEnv1Parameters.decay = 0.2f;
    modEnv1Parameters.sustain = 0.8f;
    modEnv1Parameters.release = 0.3f;
    modEnvelope1.setParameters(modEnv1Parameters);

    modEnv2Parameters.attack = 0.01f;
    modEnv2Parameters.decay = 0.2f;
    modEnv2Parameters.sustain = 0.8f;
    modEnv2Parameters.release = 0.3f;
    modEnvelope2.setParameters(modEnv2Parameters);

    // Initialize oscillators with safe defaults to match voice parameters
    oscillator1.setLevel(params.osc1Level.load());
    oscillator1.setWaveform(FreOscOscillator::Waveform::Sine);
    oscillator2.setLevel(params.osc2Level.load());
    oscillator2.setWaveform(FreOscOscillator::Waveform::Sine);
    oscillator3.setLevel(params.osc3Level.load());
    oscillator3.setWaveform(FreOscOscillator::Waveform::Sine);

    // Initialize PM modulator (will be synced with OSC3 when needed)
    pmModulator.setLevel(1.0f);
    pmModulator.setWaveform(FreOscOscillator::Waveform::Sine);

    // Initialize noise generator
    noiseGenerator.setLevel(params.noiseLevel.load());
}

FreOscVoice::~FreOscVoice()
{
}

//==============================================================================
bool FreOscVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<FreOscSound*>(sound) != nullptr;
}

void FreOscVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    juce::ignoreUnused(sound);

    currentMidiNote = midiNoteNumber;
    currentVelocity = velocity;
    noteIsOn = true;

    // Set initial pitch bend from current wheel position
    currentPitchBend = (currentPitchWheelPosition - 8192) / 8192.0f;

    // Calculate base frequency with pitch bend applied
    float pitchBendSemitones = currentPitchBend * pitchBendRange;
    float effectiveNoteNumber = midiNoteNumber + pitchBendSemitones;
    currentNoteFrequency = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(static_cast<int>(effectiveNoteNumber)));

    // If fractional note number, interpolate frequency
    if (effectiveNoteNumber != static_cast<int>(effectiveNoteNumber))
    {
        float fractionalPart = effectiveNoteNumber - static_cast<int>(effectiveNoteNumber);
        float lowerFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(static_cast<int>(effectiveNoteNumber)));
        float upperFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(static_cast<int>(effectiveNoteNumber) + 1));
        currentNoteFrequency = lowerFreq + (upperFreq - lowerFreq) * fractionalPart;
    }

    // Set up oscillators with current note frequency
    setupOscillators();
    
    // Reset oscillator phases to prevent pops from random starting phases
    oscillator1.reset();
    oscillator2.reset();
    oscillator3.reset();
    pmModulator.reset();

    // Start envelope
    envelope.noteOn();
    
    // Start modulation envelopes
    modEnvelope1.noteOn();
    modEnvelope2.noteOn();
    
    // Initialize amplitude ramping for anti-pop (20ms fade-in)
    amplitudeRamp.reset(currentSampleRate, 0.02); // 20ms ramp
    amplitudeRamp.setCurrentAndTargetValue(0.0f);
    amplitudeRamp.setTargetValue(1.0f);
    isRampingDown = false;
}

void FreOscVoice::stopNote(float velocity, bool allowTailOff)
{
    juce::ignoreUnused(velocity);

    noteIsOn = false;

    if (allowTailOff)
    {
        // Let the envelope handle the release
        envelope.noteOff();
        
        // Release modulation envelopes too
        modEnvelope1.noteOff();
        modEnvelope2.noteOff();
    }
    else
    {
        // Start amplitude ramp down for anti-pop (10ms fade-out)
        amplitudeRamp.reset(currentSampleRate, 0.01); // 10ms fade-out
        amplitudeRamp.setTargetValue(0.0f);
        isRampingDown = true;
    }
}

void FreOscVoice::pitchWheelMoved(int newPitchWheelValue)
{
    // Convert pitch wheel value (0-16383) to normalized -1.0 to +1.0 range
    currentPitchBend = (newPitchWheelValue - 8192) / 8192.0f;

    // Update oscillator frequencies with pitch bend applied
    if (noteIsOn)
    {
        setupOscillators();
    }
}

void FreOscVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
    // Convert CC value (0-127) to normalized 0.0-1.0 range
    float normalizedValue = newControllerValue / 127.0f;

    switch (controllerNumber)
    {
        case 1:   // Modulation Wheel - affects LFO amount
            ccModWheel = normalizedValue;
            break;

        case 7:   // Volume - affects overall voice level
            ccVolume = normalizedValue;
            break;

        case 11:  // Expression - additional volume control
            ccExpression = normalizedValue;
            break;

        case 71:  // Filter Resonance - modulates filter resonance
            ccFilterResonance = normalizedValue;
            break;

        case 74:  // Filter Cutoff - modulates filter frequency
            ccFilterCutoff = normalizedValue;
            break;

        default:
            // Ignore other CC numbers
            break;
    }
}

//==============================================================================
void FreOscVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    // Critical fix: Check if ALL audio sources are truly at zero to prevent phantom audio
    if (oscillator1.getCurrentLevel() <= 0.0f &&
        oscillator2.getCurrentLevel() <= 0.0f &&
        oscillator3.getCurrentLevel() <= 0.0f &&
        params.noiseLevel.load() <= 0.0f)
    {
        // All sources are off - produce absolute silence
        return;
    }

    // Process each sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Get envelope level
        float envelopeLevel = envelope.getNextSample();
        
        // Get modulation envelope levels
        float modEnv1Level = modEnvelope1.getNextSample();
        float modEnv2Level = modEnvelope2.getNextSample();

        // Get amplitude ramp value for anti-pop
        float amplitudeRampValue = amplitudeRamp.getNextValue();
        
        // If we're ramping down and reached zero, clear the note
        if (isRampingDown && amplitudeRampValue <= 0.001f)
        {
            clearCurrentNote();
            envelope.reset(); // Ensure envelope is fully reset
            break;
        }
        
        // If envelope is finished, clear the note
        if (!envelope.isActive())
        {
            clearCurrentNote();
            break;
        }

        // Apply modulation envelope modulation
        float modEnv1Mod = 0.0f, modEnv2Mod = 0.0f;
        if (params.modEnv1Amount > 0.0f && params.modEnv1Target > 0)
        {
            modEnv1Mod = modEnv1Level * params.modEnv1Amount;
        }
        if (params.modEnv2Amount > 0.0f && params.modEnv2Target > 0)
        {
            modEnv2Mod = modEnv2Level * params.modEnv2Amount;
        }

        // Generate oscillator samples
        float osc1Sample = 0.0f, osc2Sample = 0.0f, osc3Sample = 0.0f;
        float noiseSample = 0.0f;

        // Get LFO value for this sample if LFO is active
        float lfoValue = 0.0f;
        if (params.lfoAmount > 0.0f && params.lfoTarget > 0) // Check both amount and target are set (target > 0 means not "None")
        {
            // Set LFO amount (required for internal activity check)
            lfo.setAmount(params.lfoAmount);
            
            // Get the raw LFO signal and apply amount scaling
            lfoValue = lfo.getNextSample(
                static_cast<FreOscLFO::Waveform>(params.lfoWaveform.load()),
                params.lfoRate,
                static_cast<FreOscLFO::Target>(params.lfoTarget.load())
            );
            // LFO value is raw oscillator signal (-1 to +1), apply amount scaling
            lfoValue *= params.lfoAmount;
        }

        // Apply modulation envelope modulation to parameters
        float modulatedPMIndex = params.pmIndex;
        float modulatedPMRatio = params.pmRatio;
        float modulatedFilterCutoff = params.filterCutoff;
        float modulatedFilter2Cutoff = params.filter2Cutoff;
        
        // Apply ModEnv1 modulation based on target
        if (params.modEnv1Amount > 0.0f && params.modEnv1Target > 0)
        {
            switch (params.modEnv1Target.load())
            {
                case 1: modulatedPMIndex = juce::jlimit(0.0f, 10.0f, modulatedPMIndex + (modEnv1Mod * 5.0f)); break; // PM Index
                case 2: modulatedPMRatio = juce::jlimit(0.1f, 8.0f, modulatedPMRatio + (modEnv1Mod * 4.0f)); break; // PM Ratio
                case 3: modulatedFilterCutoff = juce::jlimit(0.0f, 1.0f, modulatedFilterCutoff + modEnv1Mod); break; // Filter Cutoff
                case 4: modulatedFilter2Cutoff = juce::jlimit(0.0f, 1.0f, modulatedFilter2Cutoff + modEnv1Mod); break; // Filter2 Cutoff
            }
        }
        
        // Apply ModEnv2 modulation based on target
        if (params.modEnv2Amount > 0.0f && params.modEnv2Target > 0)
        {
            switch (params.modEnv2Target.load())
            {
                case 1: modulatedPMIndex = juce::jlimit(0.0f, 10.0f, modulatedPMIndex + (modEnv2Mod * 5.0f)); break; // PM Index
                case 2: modulatedPMRatio = juce::jlimit(0.1f, 8.0f, modulatedPMRatio + (modEnv2Mod * 4.0f)); break; // PM Ratio
                case 3: modulatedFilterCutoff = juce::jlimit(0.0f, 1.0f, modulatedFilterCutoff + modEnv2Mod); break; // Filter Cutoff
                case 4: modulatedFilter2Cutoff = juce::jlimit(0.0f, 1.0f, modulatedFilter2Cutoff + modEnv2Mod); break; // Filter2 Cutoff
            }
        }

        // Apply pitch modulation from LFO if active (10% of fundamental frequency)
        float pitchModulation = 0.0f;
        bool hasPitchModulation = (params.lfoAmount > 0.0f && params.lfoTarget == 1); // Pitch target
        if (hasPitchModulation)
        {
            pitchModulation = lfoValue * 0.1f; // lfoValue already includes amount scaling
        }
        
        // Generate PM modulation signal using dedicated PM modulator
        float pmSignal = 0.0f;
        bool hasPM = (modulatedPMIndex > 0.0f);
        
        if (hasPM)
        {
            // Sync PM modulator with OSC3's waveform settings
            syncPMModulatorWithOSC3();
            
            // Set PM modulator frequency to note * ratio (independent of OSC3's frequency)
            float modulatorFreq = currentNoteFrequency * modulatedPMRatio;
            pmModulator.setFrequency(modulatorFreq);
            
            // Apply LFO pitch modulation to PM modulator if active
            if (hasPitchModulation)
                pmModulator.setFrequencyModulation(pitchModulation);
            else
                pmModulator.setFrequencyModulation(0.0f);
                
            // Generate PM modulation signal (uses OSC3's waveform but separate processing)
            float pmModulatorOutput = pmModulator.processRawSample(0.0f);
            pmSignal = pmModulatorOutput * modulatedPMIndex * 0.3f; // PM intensity controlled by Index only
        }
        
        // Process OSC3 normally for audio output (unaffected by PM)
        if (params.osc3Level > 0.0f && oscillator3.getCurrentLevel() > 0.0f)
        {
            // Apply LFO pitch modulation to OSC3 if active
            if (hasPitchModulation)
                oscillator3.setFrequencyModulation(pitchModulation);
            else
                oscillator3.setFrequencyModulation(0.0f);
                
            // OSC3 audio uses normal processing at its own frequency settings
            osc3Sample = oscillator3.processSample(0.0f);
        }

        // Generate samples from active oscillators with proper FM routing
        if (params.osc1Level > 0.0f && oscillator1.getCurrentLevel() > 0.0f)
        {
            // Apply LFO pitch modulation if active
            if (hasPitchModulation)
                oscillator1.setFrequencyModulation(pitchModulation);
            else
                oscillator1.setFrequencyModulation(0.0f);
                
            // Apply PM modulation if this oscillator is a carrier
            float pmInput = 0.0f;
            if (shouldReceivePM(1) && pmSignal != 0.0f)
            {
                pmInput = pmSignal;
            }
            
            osc1Sample = oscillator1.processSample(pmInput);
        }

        if (params.osc2Level > 0.0f && oscillator2.getCurrentLevel() > 0.0f)
        {
            // Apply LFO pitch modulation if active
            if (hasPitchModulation)
                oscillator2.setFrequencyModulation(pitchModulation);
            else
                oscillator2.setFrequencyModulation(0.0f);
                
            // Apply PM modulation if this oscillator is a carrier
            float pmInput = 0.0f;
            if (shouldReceivePM(2) && pmSignal != 0.0f)
            {
                pmInput = pmSignal;
            }
            osc2Sample = oscillator2.processSample(pmInput);
        }

        // OSC3 was processed above independently of PM

        // Generate noise if active
        if (params.noiseLevel > 0.0f)
        {
            noiseSample = noiseGenerator.processSample();
        }

        // Apply volume modulation from LFO if active
        float volumeModulation = 1.0f;
        if (params.lfoAmount > 0.0f && params.lfoTarget == 4) // Volume target
        {
            volumeModulation = 1.0f + (lfoValue * 0.5f); // lfoValue already includes amount scaling
            volumeModulation = juce::jmax(0.0f, volumeModulation); // Prevent negative volume
        }

        // Mix all sources
        float mixedSample = (osc1Sample + osc2Sample + osc3Sample + noiseSample) * volumeModulation;

        // Safety check: prevent NaN/infinity values that could cause crackling
        if (!std::isfinite(mixedSample))
            mixedSample = 0.0f;

        // Apply envelope and CC volume/expression modulation
        float ccVolumeModulation = ccVolume * ccExpression;

        // Add modulation wheel effect on LFO amount
        float effectiveLfoAmount = params.lfoAmount;
        if (ccModWheel > 0.0f)
        {
            effectiveLfoAmount += ccModWheel * 0.5f; // Mod wheel can add up to 50% more LFO
        }

        mixedSample *= envelopeLevel * currentVelocity * ccVolumeModulation * amplitudeRampValue;

        // Apply per-voice filtering (after envelope, before panning)
        // Check for LFO filter modulation
        float filterModulation = 0.0f;
        if (params.lfoAmount > 0.0f && params.lfoTarget == 2) // Filter target
        {
            filterModulation = lfoValue * 0.3f; // ±30% modulation range
        }

        // Apply filter modulation to cutoff (starting from mod envelope modulated value)
        float finalModulatedCutoff = modulatedFilterCutoff + filterModulation;
        finalModulatedCutoff = juce::jlimit(0.0f, 1.0f, finalModulatedCutoff);

        // Check for LFO filter 2 modulation
        float filter2Modulation = 0.0f;
        if (params.lfoAmount > 0.0f && params.lfoTarget == 3) // Filter2 target
        {
            filter2Modulation = lfoValue * 0.3f; // ±30% modulation range
        }

        // Apply filter2 modulation to cutoff (starting from mod envelope modulated value)
        float finalModulatedCutoff2 = modulatedFilter2Cutoff + filter2Modulation;
        finalModulatedCutoff2 = juce::jlimit(0.0f, 1.0f, finalModulatedCutoff2);

        // Update filter if modulation changed or mod envelope is active
        if (std::abs(filterModulation) > 0.001f || finalModulatedCutoff != params.filterCutoff.load())
        {
            voiceFilter.setCutoffFrequency(finalModulatedCutoff);
        }
        
        // Update filter2 if modulation changed or mod envelope is active
        if (std::abs(filter2Modulation) > 0.001f || finalModulatedCutoff2 != params.filter2Cutoff.load())
        {
            voiceFilter2.setCutoffFrequency(finalModulatedCutoff2);
        }

        // Process sample through dual filter system
        FilterRouting routing = static_cast<FilterRouting>(params.filterRouting.load());
        
        float filteredSample = mixedSample;
        
        if (routing == FilterOff)
        {
            // Only Filter 1 processes audio
            float tempSample = filteredSample;
            float* samplePtr = &tempSample;
            juce::dsp::AudioBlock<float> filterBlock(&samplePtr, 1, 1);
            juce::dsp::ProcessContextReplacing<float> filterContext(filterBlock);
            voiceFilter.process(filterContext);
            filteredSample = tempSample;
        }
        else if (routing == FilterParallel)
        {
            // Both filters process in parallel, outputs summed
            float tempSample1 = filteredSample;
            float tempSample2 = filteredSample;
            
            // Process through Filter 1
            float* samplePtr1 = &tempSample1;
            juce::dsp::AudioBlock<float> filterBlock1(&samplePtr1, 1, 1);
            juce::dsp::ProcessContextReplacing<float> filterContext1(filterBlock1);
            voiceFilter.process(filterContext1);
            
            // Process through Filter 2
            float* samplePtr2 = &tempSample2;
            juce::dsp::AudioBlock<float> filterBlock2(&samplePtr2, 1, 1);
            juce::dsp::ProcessContextReplacing<float> filterContext2(filterBlock2);
            voiceFilter2.process(filterContext2);
            
            // Sum the parallel outputs (with 0.5 scaling to prevent clipping)
            filteredSample = (tempSample1 + tempSample2) * 0.5f;
        }
        else if (routing == FilterSeries)
        {
            // Filter 1 -> Filter 2 in series
            float tempSample = filteredSample;
            
            // Process through Filter 1 first
            float* samplePtr1 = &tempSample;
            juce::dsp::AudioBlock<float> filterBlock1(&samplePtr1, 1, 1);
            juce::dsp::ProcessContextReplacing<float> filterContext1(filterBlock1);
            voiceFilter.process(filterContext1);
            
            // Then process through Filter 2
            float* samplePtr2 = &tempSample;
            juce::dsp::AudioBlock<float> filterBlock2(&samplePtr2, 1, 1);
            juce::dsp::ProcessContextReplacing<float> filterContext2(filterBlock2);
            voiceFilter2.process(filterContext2);
            
            filteredSample = tempSample;
        }
        
        mixedSample = filteredSample;

        // Scale down for polyphony to prevent clipping when multiple voices play
        // Very conservative scaling since we fixed the triple-level issue
        mixedSample *= 0.3f; // Conservative but not too quiet

        // Final safety check after all processing
        if (!std::isfinite(mixedSample))
            mixedSample = 0.0f;

        // Apply DC blocking filter to remove any DC offset
        mixedSample = dcBlocker.processSample(mixedSample);

        // Soft clipping to prevent harsh distortion
        mixedSample = juce::jlimit(-1.0f, 1.0f, mixedSample);

        // Calculate stereo positioning with LFO pan modulation
        float leftGain = 1.0f, rightGain = 1.0f;

        // Calculate base panning (weighted average based on oscillator levels)
        float totalLevelForPan = params.osc1Level + params.osc2Level + params.osc3Level;
        float avgPan = 0.0f;
        if (totalLevelForPan > 0.0f)
        {
            avgPan = (params.osc1Pan * params.osc1Level +
                     params.osc2Pan * params.osc2Level +
                     params.osc3Pan * params.osc3Level) / totalLevelForPan;
        }

        // Apply LFO pan modulation if active
        if (params.lfoAmount > 0.0f && params.lfoTarget == 5) // Pan target
        {
            avgPan += lfoValue; // lfoValue already includes amount scaling
            avgPan = juce::jlimit(-1.0f, 1.0f, avgPan); // Clamp to valid range
        }

        // Apply constant power panning (corrected algorithm)
        // Pan value: -1.0 = full left, 0.0 = center, +1.0 = full right
        const float panAngle = (avgPan + 1.0f) * juce::MathConstants<float>::pi / 4.0f;
        leftGain = std::cos(panAngle);
        rightGain = std::sin(panAngle);

        // Output to stereo buffer
        outputBuffer.addSample(0, startSample + sample, mixedSample * leftGain);
        if (outputBuffer.getNumChannels() > 1)
            outputBuffer.addSample(1, startSample + sample, mixedSample * rightGain);
    }
}

//==============================================================================
bool FreOscVoice::isVoiceActive() const
{
    return envelope.isActive();
}

void FreOscVoice::setCurrentPlaybackSampleRate(double sampleRate)
{
    currentSampleRate = sampleRate;

    // Prepare all DSP components
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;

    oscillator1.prepare(spec);
    oscillator2.prepare(spec);
    oscillator3.prepare(spec);
    pmModulator.prepare(spec);
    noiseGenerator.prepare(sampleRate);
    lfo.prepare(sampleRate);

    // Prepare per-voice filters
    voiceFilter.prepare(spec);
    voiceFilter2.prepare(spec);

    envelope.setSampleRate(sampleRate);
    
    // Set sample rate for modulation envelopes
    modEnvelope1.setSampleRate(sampleRate);
    modEnvelope2.setSampleRate(sampleRate);
    
    // Initialize amplitude ramp for anti-pop
    amplitudeRamp.reset(sampleRate, 0.02); // 20ms default ramp time
    amplitudeRamp.setCurrentAndTargetValue(1.0f);
    
    // Initialize DC blocking high-pass filter (cutoff around 5Hz)
    juce::dsp::ProcessSpec dcSpec;
    dcSpec.sampleRate = sampleRate;
    dcSpec.maximumBlockSize = 1;
    dcSpec.numChannels = 1;
    dcBlocker.prepare(dcSpec);
    auto coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 5.0f);
    dcBlocker.coefficients = coefficients;
    dcBlocker.reset();
}

//==============================================================================
// Parameter update methods
void FreOscVoice::updateOscillatorParameters(
    int osc1Waveform, int osc1Octave, float osc1Level, float osc1Detune, float osc1Pan,
    int osc2Waveform, int osc2Octave, float osc2Level, float osc2Detune, float osc2Pan,
    int osc3Waveform, int osc3Octave, float osc3Level, float osc3Detune, float osc3Pan)
{
    // Update oscillator 1
    oscillator1.setWaveform(static_cast<FreOscOscillator::Waveform>(osc1Waveform));
    oscillator1.setOctave(osc1Octave);
    oscillator1.setLevel(osc1Level);
    oscillator1.setDetune(osc1Detune);
    params.osc1Level = osc1Level;
    params.osc1Pan = osc1Pan;

    // Update oscillator 2
    oscillator2.setWaveform(static_cast<FreOscOscillator::Waveform>(osc2Waveform));
    oscillator2.setOctave(osc2Octave);
    oscillator2.setLevel(osc2Level);
    oscillator2.setDetune(osc2Detune);
    params.osc2Level = osc2Level;
    params.osc2Pan = osc2Pan;

    // Update oscillator 3
    oscillator3.setWaveform(static_cast<FreOscOscillator::Waveform>(osc3Waveform));
    oscillator3.setOctave(osc3Octave);
    oscillator3.setLevel(osc3Level);
    oscillator3.setDetune(osc3Detune);
    params.osc3Level = osc3Level;
    params.osc3Pan = osc3Pan;

    // Recalculate frequencies if note is active
    if (noteIsOn)
        setupOscillators();
}

void FreOscVoice::updateNoiseParameters(int noiseType, float noiseLevel, float noisePan)
{
    noiseGenerator.setNoiseType(static_cast<FreOscNoiseGenerator::NoiseType>(noiseType));
    noiseGenerator.setLevel(noiseLevel);
    noiseGenerator.setPan(noisePan);
    params.noiseLevel = noiseLevel;
    params.noisePan = noisePan;
}

void FreOscVoice::updateEnvelopeParameters(float attack, float decay, float sustain, float release)
{
    envelopeParameters.attack = attack;
    envelopeParameters.decay = decay;
    envelopeParameters.sustain = sustain;
    envelopeParameters.release = release;
    envelope.setParameters(envelopeParameters);
}

void FreOscVoice::updatePMParameters(float pmIndex, int pmCarrier, float pmRatio)
{
    params.pmIndex = pmIndex;
    params.pmCarrier = pmCarrier;
    params.pmRatio = pmRatio;
    // Note: OSC3 is always the message signal source
}

void FreOscVoice::updateLFOParameters(int lfoWaveform, float lfoRate, int lfoTarget, float lfoAmount)
{
    params.lfoWaveform = lfoWaveform;
    params.lfoRate = lfoRate;
    params.lfoTarget = lfoTarget;
    params.lfoAmount = lfoAmount;
}

void FreOscVoice::updateFilterParameters(int filterType, float cutoff, float resonance, float gain)
{
    params.filterType = filterType;
    params.filterCutoff = cutoff;
    params.filterResonance = resonance;
    params.filterGain = gain;

    // Update the voice filter immediately
    voiceFilter.setFilterType(static_cast<FreOscFilter::FilterType>(filterType));
    voiceFilter.setCutoffFrequency(cutoff);
    voiceFilter.setResonance(resonance);
    voiceFilter.setGain(gain);
}

void FreOscVoice::updateFilter2Parameters(int filter2Type, float cutoff2, float resonance2, float gain2)
{
    params.filter2Type = filter2Type;
    params.filter2Cutoff = cutoff2;
    params.filter2Resonance = resonance2;
    params.filter2Gain = gain2;

    // Update the voice filter 2 immediately
    voiceFilter2.setFilterType(static_cast<FreOscFilter::FilterType>(filter2Type));
    voiceFilter2.setCutoffFrequency(cutoff2);
    voiceFilter2.setResonance(resonance2);
    voiceFilter2.setGain(gain2);
}

void FreOscVoice::updateFilterRouting(int routing)
{
    params.filterRouting = routing;
}

void FreOscVoice::updateModEnv1Parameters(float attack, float decay, float sustain, float release, float amount, int target)
{
    // Update envelope parameters
    modEnv1Parameters.attack = attack;
    modEnv1Parameters.decay = decay;
    modEnv1Parameters.sustain = sustain;
    modEnv1Parameters.release = release;
    modEnvelope1.setParameters(modEnv1Parameters);

    // Update modulation parameters
    params.modEnv1Amount = amount;
    params.modEnv1Target = target;
}

void FreOscVoice::updateModEnv2Parameters(float attack, float decay, float sustain, float release, float amount, int target)
{
    // Update envelope parameters
    modEnv2Parameters.attack = attack;
    modEnv2Parameters.decay = decay;
    modEnv2Parameters.sustain = sustain;
    modEnv2Parameters.release = release;
    modEnvelope2.setParameters(modEnv2Parameters);

    // Update modulation parameters
    params.modEnv2Amount = amount;
    params.modEnv2Target = target;
}

//==============================================================================
// Helper methods
void FreOscVoice::setupOscillators()
{
    // Set base frequency for all oscillators
    oscillator1.setFrequency(currentNoteFrequency);
    oscillator2.setFrequency(currentNoteFrequency);
    oscillator3.setFrequency(currentNoteFrequency);
}

void FreOscVoice::calculateNoteFrequency(int midiNote, int octaveOffset, float detuneAmount)
{
    float baseFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNote));
    float octaveMultiplier = std::pow(2.0f, static_cast<float>(octaveOffset));
    float detuneRatio = std::pow(2.0f, detuneAmount / 1200.0f);

    // Return calculated frequency (this would be used by individual oscillators)
    juce::ignoreUnused(baseFreq, octaveMultiplier, detuneRatio);
}

float FreOscVoice::getPMModulationSignal()
{
    // PM processing is now handled inline in renderNextBlock() for better coordination
    // This method is kept for compatibility but not used
    return 0.0f;
}

void FreOscVoice::syncPMModulatorWithOSC3()
{
    // Copy OSC3's waveform and settings to PM modulator for consistent character
    pmModulator.setWaveform(oscillator3.getCurrentWaveform());
    
    // Copy OSC3's octave and detune settings so PM character matches OSC3
    // This ensures when user changes OSC3 octave/detune, PM modulator reflects the changes
    pmModulator.setOctave(oscillator3.getCurrentOctave());
    pmModulator.setDetune(oscillator3.getCurrentDetune());
    
    pmModulator.setLevel(1.0f); // PM modulator always at full level for raw waveform
}

bool FreOscVoice::shouldReceivePM(int oscillatorIndex)
{
    // Check if the specified oscillator should receive PM modulation
    // OSC3 is always the message signal source, carriers are Osc1, Osc2, or Both
    int pmCarrier = params.pmCarrier.load();

    switch (pmCarrier)
    {
        case 0: // Oscillator 1 only
            return oscillatorIndex == 1;
        case 1: // Oscillator 2 only
            return oscillatorIndex == 2;
        case 2: // Both Osc 1 & 2
            return (oscillatorIndex == 1 || oscillatorIndex == 2);
        default:
            return false;
    }
}