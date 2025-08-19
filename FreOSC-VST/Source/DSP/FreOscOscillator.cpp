#include "FreOscOscillator.h"

//==============================================================================
FreOscOscillator::FreOscOscillator()
{
    // Initialize oscillator with sine wave by default
    oscillator.initialise([](float x) { return std::sin(x); });
}

FreOscOscillator::~FreOscOscillator()
{
}

//==============================================================================
void FreOscOscillator::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    oscillator.prepare(spec);

    // Calculate phase increment for manual phase accumulation
    phaseIncrement = finalFrequency * 2.0f * juce::MathConstants<float>::pi / static_cast<float>(sampleRate);

    reset();
}

void FreOscOscillator::reset()
{
    oscillator.reset();
    phase = 0.0f;
}

//==============================================================================
void FreOscOscillator::setWaveform(Waveform waveform)
{
    if (currentWaveform != waveform)
    {
        currentWaveform = waveform;
        updateOscillatorWaveform();
    }
}

void FreOscOscillator::setFrequency(float frequency)
{
    baseFrequency = frequency;
    updateFinalFrequency();
}

void FreOscOscillator::setLevel(float newLevel)
{
    level = juce::jlimit(0.0f, 1.0f, newLevel);
}

void FreOscOscillator::setOctave(int octave)
{
    octaveOffset = juce::jlimit(-2, 2, octave);
    updateFinalFrequency();
}

void FreOscOscillator::setDetune(float cents)
{
    detuneAmount = juce::jlimit(-50.0f, 50.0f, cents);
    updateFinalFrequency();
}

void FreOscOscillator::setFrequencyModulation(float modAmount)
{
    frequencyModulation = modAmount;
}

//==============================================================================
float FreOscOscillator::processSample(float fmInput)
{
    if (level <= 0.0f)
        return 0.0f;

    // Calculate current phase increment including frequency modulation
    float currentPhaseInc = phaseIncrement * (1.0f + frequencyModulation);

    // Advance phase
    phase += currentPhaseInc;

    // Wrap main phase to prevent overflow
    while (phase >= 2.0f * juce::MathConstants<float>::pi)
        phase -= 2.0f * juce::MathConstants<float>::pi;

    // Apply phase modulation (FM input)
    float modulatedPhase = phase + fmInput;

    // Wrap phase to [0, 2*pi] range
    while (modulatedPhase >= 2.0f * juce::MathConstants<float>::pi)
        modulatedPhase -= 2.0f * juce::MathConstants<float>::pi;
    while (modulatedPhase < 0.0f)
        modulatedPhase += 2.0f * juce::MathConstants<float>::pi;

    // Generate waveform sample
    float sample = generateWaveformSample(modulatedPhase);
    return sample * level;
}

float FreOscOscillator::processRawSample(float fmInput)
{
    // Same processing as processSample() but returns raw waveform without level scaling
    // This is used for PM modulation where we want the waveform character but not the level
    
    // Always process even if level is 0 - PM needs the raw waveform
    // Calculate current phase increment including frequency modulation
    float currentPhaseInc = phaseIncrement * (1.0f + frequencyModulation);
    
    // Advance phase
    phase += currentPhaseInc;
    
    // Wrap main phase to prevent overflow
    while (phase >= 2.0f * juce::MathConstants<float>::pi)
        phase -= 2.0f * juce::MathConstants<float>::pi;
        
    // Apply phase modulation (FM input)
    float modulatedPhase = phase + fmInput;
    
    // Wrap phase to [0, 2*pi] range
    while (modulatedPhase >= 2.0f * juce::MathConstants<float>::pi)
        modulatedPhase -= 2.0f * juce::MathConstants<float>::pi;
    while (modulatedPhase < 0.0f)
        modulatedPhase += 2.0f * juce::MathConstants<float>::pi;
        
    // Generate and return raw waveform sample (no level scaling)
    return generateWaveformSample(modulatedPhase);
}

void FreOscOscillator::processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, float fmInput)
{
    if (level <= 0.0f)
        return;

    // Apply FM modulation if provided
    float currentFreq = finalFrequency;
    if (fmInput != 0.0f)
        currentFreq += fmInput;

    oscillator.setFrequency(currentFreq);

    // Process block with level scaling
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float oscillatorSample = oscillator.processSample(0.0f) * level;

        // Add to both stereo channels
        buffer.addSample(0, startSample + sample, oscillatorSample);
        buffer.addSample(1, startSample + sample, oscillatorSample);
    }
}

//==============================================================================
void FreOscOscillator::updateFinalFrequency()
{
    // Calculate final frequency: base * octave_multiplier * detune_ratio
    float octaveMultiplier = octaveToMultiplier(octaveOffset);
    float detuneRatio = centsToRatio(detuneAmount);

    finalFrequency = baseFrequency * octaveMultiplier * detuneRatio;

    // Update the oscillator frequency
    oscillator.setFrequency(finalFrequency);

    // Update phase increment for manual phase accumulation
    if (sampleRate > 0.0)
        phaseIncrement = finalFrequency * 2.0f * juce::MathConstants<float>::pi / static_cast<float>(sampleRate);
}

void FreOscOscillator::updateOscillatorWaveform()
{
    // Set the oscillator waveform based on current selection
    switch (currentWaveform)
    {
        case Waveform::Sine:
            oscillator.initialise([](float x) { return std::sin(x); });
            break;

        case Waveform::Square:
            oscillator.initialise([](float x) { return x < 0.0f ? -1.0f : 1.0f; });
            break;

        case Waveform::Sawtooth:
            oscillator.initialise([](float x) {
                return juce::jmap(x, -juce::MathConstants<float>::pi, juce::MathConstants<float>::pi, -1.0f, 1.0f);
            });
            break;

        case Waveform::Triangle:
            oscillator.initialise([](float x) {
                return juce::jmap(std::abs(x), 0.0f, juce::MathConstants<float>::pi, -1.0f, 1.0f);
            });
            break;
    }
}

float FreOscOscillator::generateWaveformSample(float phaseValue) const
{
    switch (currentWaveform)
    {
        case Waveform::Sine:
            return std::sin(phaseValue);

        case Waveform::Square:
            return (phaseValue < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;

        case Waveform::Sawtooth:
            return juce::jmap(phaseValue, 0.0f, 2.0f * juce::MathConstants<float>::pi, -1.0f, 1.0f);

        case Waveform::Triangle:
            if (phaseValue < juce::MathConstants<float>::pi)
                return juce::jmap(phaseValue, 0.0f, juce::MathConstants<float>::pi, -1.0f, 1.0f);
            else
                return juce::jmap(phaseValue, juce::MathConstants<float>::pi, 2.0f * juce::MathConstants<float>::pi, 1.0f, -1.0f);

        default:
            return 0.0f;
    }
}