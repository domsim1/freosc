#include "FreOscLFO.h"

//==============================================================================
FreOscLFO::FreOscLFO()
{
    // Initialize oscillator with sine wave
    oscillator.initialise([](float x) { return std::sin(x); });
    
    // Initialize random generator
    random.setSeedRandomly();
}

FreOscLFO::~FreOscLFO()
{
}

//==============================================================================
void FreOscLFO::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    
    // Prepare JUCE oscillator
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;
    oscillator.prepare(spec);
    
    // Update phase increment and random step size
    updatePhaseIncrement();
    samplesPerRandomStep = static_cast<int>(sampleRate / (rate * 20.0f)); // 20 steps per cycle
    
    reset();
}

void FreOscLFO::reset()
{
    oscillator.reset();
    phase = 0.0f;
    randomValue = 0.0f;
    samplesSinceLastRandom = 0;
}

//==============================================================================
void FreOscLFO::setWaveform(Waveform waveform)
{
    if (currentWaveform != waveform)
    {
        currentWaveform = waveform;
        
        // Update JUCE oscillator waveform for standard types
        switch (waveform)
        {
            case Waveform::Sine:
                oscillator.initialise([](float x) { return std::sin(x); });
                break;
            case Waveform::Triangle:
                oscillator.initialise([](float x) { 
                    return juce::jmap(std::abs(x), 0.0f, juce::MathConstants<float>::pi, -1.0f, 1.0f); 
                });
                break;
            case Waveform::Sawtooth:
                oscillator.initialise([](float x) { 
                    return juce::jmap(x, -juce::MathConstants<float>::pi, juce::MathConstants<float>::pi, -1.0f, 1.0f); 
                });
                break;
            case Waveform::Square:
                oscillator.initialise([](float x) { return x < 0.0f ? -1.0f : 1.0f; });
                break;
            case Waveform::Random:
                // Random uses custom generation, no need to set oscillator
                break;
        }
    }
}

void FreOscLFO::setRate(float rateHz)
{
    rate = juce::jlimit(0.01f, 20.0f, rateHz);
    updatePhaseIncrement();
    
    // Update random step size for sample and hold
    if (sampleRate > 0.0)
    {
        samplesPerRandomStep = juce::jmax(1, static_cast<int>(sampleRate / (rate * 20.0f)));
    }
}

void FreOscLFO::setTarget(Target target)
{
    currentTarget = target;
}

void FreOscLFO::setAmount(float newAmount)
{
    amount = juce::jlimit(0.0f, 1.0f, newAmount);
}

//==============================================================================
float FreOscLFO::getNextSample(Waveform waveform, float rateHz, Target target)
{
    // Update parameters if changed
    if (rateHz != rate)
        setRate(rateHz);
    if (waveform != currentWaveform)
        setWaveform(waveform);
    if (target != currentTarget)
        setTarget(target);
    
    // Check if LFO should be active (amount > 0 and target is not None)
    if (amount <= 0.0f || currentTarget == Target::None)
        return 0.0f;
    
    float sample = 0.0f;
    
    switch (currentWaveform)
    {
        case Waveform::Sine:
            sample = generateSine();
            break;
        case Waveform::Triangle:
            sample = generateTriangle();
            break;
        case Waveform::Sawtooth:
            sample = generateSawtooth();
            break;
        case Waveform::Square:
            sample = generateSquare();
            break;
        case Waveform::Random:
            sample = generateRandom();
            break;
    }
    
    return sample;
}

//==============================================================================
// Waveform generation methods

float FreOscLFO::generateSine()
{
    float sample = std::sin(phase);
    phase += phaseIncrement;
    if (phase >= juce::MathConstants<float>::twoPi)
        phase -= juce::MathConstants<float>::twoPi;
    return sample;
}

float FreOscLFO::generateTriangle()
{
    float sample;
    if (phase < juce::MathConstants<float>::pi)
        sample = juce::jmap(phase, 0.0f, juce::MathConstants<float>::pi, -1.0f, 1.0f);
    else
        sample = juce::jmap(phase, juce::MathConstants<float>::pi, juce::MathConstants<float>::twoPi, 1.0f, -1.0f);
    
    phase += phaseIncrement;
    if (phase >= juce::MathConstants<float>::twoPi)
        phase -= juce::MathConstants<float>::twoPi;
    return sample;
}

float FreOscLFO::generateSawtooth()
{
    float sample = juce::jmap(phase, 0.0f, juce::MathConstants<float>::twoPi, -1.0f, 1.0f);
    phase += phaseIncrement;
    if (phase >= juce::MathConstants<float>::twoPi)
        phase -= juce::MathConstants<float>::twoPi;
    return sample;
}

float FreOscLFO::generateSquare()
{
    float sample = (phase < juce::MathConstants<float>::pi) ? -1.0f : 1.0f;
    phase += phaseIncrement;
    if (phase >= juce::MathConstants<float>::twoPi)
        phase -= juce::MathConstants<float>::twoPi;
    return sample;
}

float FreOscLFO::generateRandom()
{
    // Sample and hold random LFO (matching JavaScript implementation)
    if (samplesSinceLastRandom >= samplesPerRandomStep)
    {
        randomValue = random.nextFloat() * 2.0f - 1.0f;
        samplesSinceLastRandom = 0;
    }
    
    samplesSinceLastRandom++;
    return randomValue;
}

//==============================================================================
void FreOscLFO::updatePhaseIncrement()
{
    if (sampleRate > 0.0)
        phaseIncrement = static_cast<float>(juce::MathConstants<double>::twoPi * rate / sampleRate);
    else
        phaseIncrement = 0.0f;
}