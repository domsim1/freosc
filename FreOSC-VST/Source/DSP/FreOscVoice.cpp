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

    // Initialize oscillators with safe defaults to match voice parameters
    oscillator1.setLevel(params.osc1Level.load());
    oscillator1.setWaveform(FreOscOscillator::Waveform::Sine);
    oscillator2.setLevel(params.osc2Level.load());
    oscillator2.setWaveform(FreOscOscillator::Waveform::Sine);
    oscillator3.setLevel(params.osc3Level.load());
    oscillator3.setWaveform(FreOscOscillator::Waveform::Sine);

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

    // Initialize FM oscillator with sine wave (standard for FM synthesis)
    fmOscillator.setWaveform(FreOscOscillator::Waveform::Sine);
    fmOscillator.setLevel(1.0f);
    fmOscillator.reset(); // Reset FM oscillator phase too

    // Start envelope
    envelope.noteOn();
    
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

        // Generate oscillator samples
        float osc1Sample = 0.0f, osc2Sample = 0.0f, osc3Sample = 0.0f;
        float noiseSample = 0.0f;

        // Get LFO value for this sample if LFO is active
        float lfoValue = 0.0f;
        if (params.lfoAmount > 0.0f && params.lfoTarget > 0) // Check both amount and target are set (target > 0 means not "None")
        {
            // Set LFO amount and get the raw LFO signal
            lfo.setAmount(params.lfoAmount);
            lfoValue = lfo.getNextSample(
                static_cast<FreOscLFO::Waveform>(params.lfoWaveform.load()),
                params.lfoRate,
                static_cast<FreOscLFO::Target>(params.lfoTarget.load())
            );
            // LFO value is now the raw oscillator signal (-1 to +1), apply amount scaling
            lfoValue *= params.lfoAmount;
        }

        // Get FM modulation signal if active - always uses Oscillator 3 as source
        // Note: FM works independently of Osc3's audio level
        float fmSignal = 0.0f;
        if (params.fmAmount > 0.0f) // Only need FM amount > 0, not Osc3 audio level
        {
            fmSignal = getFMModulationSignal();
        }

        // Apply pitch modulation from LFO if active (10% of fundamental frequency)
        float pitchModulation = 0.0f;
        bool hasPitchModulation = (params.lfoAmount > 0.0f && params.lfoTarget == 1); // Pitch target
        if (hasPitchModulation)
        {
            pitchModulation = lfoValue * 0.1f; // lfoValue already includes amount scaling
        }

        // Generate samples from active oscillators with proper FM routing
        // Double-check both voice params and oscillator level for safety
        if (params.osc1Level > 0.0f && oscillator1.getCurrentLevel() > 0.0f)
        {
            // Apply LFO pitch modulation only if active
            if (hasPitchModulation)
                oscillator1.setFrequencyModulation(pitchModulation);
            else
                oscillator1.setFrequencyModulation(0.0f);

            // Apply FM modulation if this oscillator is a target (fmSignal is already scaled)
            float fmAmount = shouldReceiveFM(1) ? fmSignal : 0.0f;
            osc1Sample = oscillator1.processSample(fmAmount);
        }

        if (params.osc2Level > 0.0f && oscillator2.getCurrentLevel() > 0.0f)
        {
            if (hasPitchModulation)
                oscillator2.setFrequencyModulation(pitchModulation);
            else
                oscillator2.setFrequencyModulation(0.0f);
            float fmAmount = shouldReceiveFM(2) ? fmSignal : 0.0f;
            osc2Sample = oscillator2.processSample(fmAmount);
        }

        if (params.osc3Level > 0.0f && oscillator3.getCurrentLevel() > 0.0f)
        {
            if (hasPitchModulation)
                oscillator3.setFrequencyModulation(pitchModulation);
            else
                oscillator3.setFrequencyModulation(0.0f);
            float fmAmount = shouldReceiveFM(3) ? fmSignal : 0.0f;
            osc3Sample = oscillator3.processSample(fmAmount);
        }

        // Generate noise if active
        if (params.noiseLevel > 0.0f)
        {
            noiseSample = noiseGenerator.processSample();
        }

        // Apply volume modulation from LFO if active
        float volumeModulation = 1.0f;
        if (params.lfoAmount > 0.0f && params.lfoTarget == 3) // Volume target
        {
            volumeModulation = 1.0f + (lfoValue * 0.5f); // lfoValue already includes amount scaling
            volumeModulation = juce::jmax(0.0f, volumeModulation); // Prevent negative volume
        }

        // Mix all sources - oscillators already apply their individual levels internally
        // No additional scaling needed since each oscillator handles its own level
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

        // Apply filter modulation to cutoff
        float modulatedCutoff = params.filterCutoff + filterModulation;
        modulatedCutoff = juce::jlimit(0.0f, 1.0f, modulatedCutoff);

        // Update filter if modulation changed
        if (std::abs(filterModulation) > 0.001f)
        {
            voiceFilter.setCutoffFrequency(modulatedCutoff);
        }

        // Process sample through per-voice filter
        // Create a temporary buffer for single sample processing
        float tempSample = mixedSample;
        float* samplePtr = &tempSample;
        juce::dsp::AudioBlock<float> filterBlock(&samplePtr, 1, 1);
        juce::dsp::ProcessContextReplacing<float> filterContext(filterBlock);
        voiceFilter.process(filterContext);
        mixedSample = tempSample;

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
        if (params.lfoAmount > 0.0f && params.lfoTarget == 4) // Pan target
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
    fmOscillator.prepare(spec);  // Initialize FM oscillator
    noiseGenerator.prepare(sampleRate);
    lfo.prepare(sampleRate);

    // Prepare per-voice filter
    voiceFilter.prepare(spec);

    envelope.setSampleRate(sampleRate);
    
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

void FreOscVoice::updateFMParameters(float fmAmount, int fmSource, int fmTarget, float fmRatio)
{
    params.fmAmount = fmAmount;
    params.fmSource = fmSource; // Still store it for compatibility, but ignore in processing
    params.fmTarget = fmTarget;
    params.fmRatio = fmRatio;
    // Note: Implementation always uses Oscillator 3 as source regardless of fmSource parameter
}

void FreOscVoice::updateLFOParameters(int lfoWaveform, float lfoRate, int lfoTarget, float lfoAmount)
{
    params.lfoWaveform = lfoWaveform;
    params.lfoRate = lfoRate;
    params.lfoTarget = lfoTarget;
    params.lfoAmount = lfoAmount;
}

void FreOscVoice::updateFilterParameters(int filterType, float cutoff, float resonance, float gain, int formantVowel)
{
    params.filterType = filterType;
    params.filterCutoff = cutoff;
    params.filterResonance = resonance;
    params.filterGain = gain;
    params.formantVowel = formantVowel;

    // Update the voice filter immediately
    voiceFilter.setFilterType(static_cast<FreOscFilter::FilterType>(filterType));
    voiceFilter.setCutoffFrequency(cutoff);
    voiceFilter.setResonance(resonance);
    voiceFilter.setGain(gain);
    voiceFilter.setFormantVowel(static_cast<FreOscFilter::FormantVowel>(formantVowel));
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

float FreOscVoice::getFMModulationSignal()
{
    // Generate FM modulation using Oscillator 3 as the fixed source
    // FM modulation works independently of Oscillator 3's audio output level

    if (params.fmAmount <= 0.0f)
        return 0.0f;

    // Always use Oscillator 3's frequency as the FM source reference
    // This works even when Osc3's audio level is zero
    float sourceFreq = oscillator3.getCurrentFrequency();

    // Calculate and set the FM frequency based on the ratio
    float fmFrequency = sourceFreq * params.fmRatio;
    fmOscillator.setFrequency(fmFrequency);

    // Generate the FM modulation signal with proper scaling for phase modulation
    // In phase modulation, the modulation depth is directly the phase deviation in radians
    float modulator = fmOscillator.processSample();
    float phaseDeviation = params.fmAmount * 0.1f; // Scale FM amount to reasonable phase modulation range
    return modulator * phaseDeviation;
}

bool FreOscVoice::shouldReceiveFM(int oscillatorIndex)
{
    // Check if the specified oscillator should receive FM modulation
    // Updated for new FM routing: Osc3 is always source, targets are Osc1, Osc2, or Both
    int fmTarget = params.fmTarget.load();

    switch (fmTarget)
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