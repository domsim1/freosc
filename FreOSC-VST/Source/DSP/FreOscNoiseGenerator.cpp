#include "FreOscNoiseGenerator.h"

//==============================================================================
FreOscNoiseGenerator::FreOscNoiseGenerator()
{
    // Initialize with a random seed
    random.setSeedRandomly();
}

FreOscNoiseGenerator::~FreOscNoiseGenerator()
{
}

//==============================================================================
void FreOscNoiseGenerator::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    reset();
}

void FreOscNoiseGenerator::reset()
{
    // Reset all filter states
    pinkB0 = pinkB1 = pinkB2 = pinkB3 = pinkB4 = pinkB5 = pinkB6 = 0.0f;
    brownState = 0.0f;
    blueLastOut = 0.0f;
    violetLastOut = violetLastOut2 = 0.0f;
    greyB0 = greyB1 = greyB2 = greyB3 = greyB4 = greyB5 = 0.0f;
    digitalLastSample = 0.0f;
    windB0 = windB1 = windB2 = 0.0f;
    windModPhase = 0.0f;
    oceanB0 = oceanB1 = oceanB2 = 0.0f;
    oceanWavePhase = 0.0f;
    oceanWaveAmp = 0.0f;
}

//==============================================================================
void FreOscNoiseGenerator::setNoiseType(NoiseType type)
{
    if (currentType != type)
    {
        currentType = type;
        // Reset states when switching noise types to avoid artifacts
        reset();
    }
}

void FreOscNoiseGenerator::setLevel(float newLevel)
{
    level = juce::jlimit(0.0f, 1.0f, newLevel);
}

void FreOscNoiseGenerator::setPan(float newPan)
{
    pan = juce::jlimit(-1.0f, 1.0f, newPan);
}

//==============================================================================
float FreOscNoiseGenerator::processSample()
{
    if (level <= 0.0f)
        return 0.0f;

    float sample = 0.0f;

    switch (currentType)
    {
        case NoiseType::White:   sample = generateWhiteNoise(); break;
        case NoiseType::Pink:    sample = generatePinkNoise(); break;
        case NoiseType::Brown:   sample = generateBrownNoise(); break;
        case NoiseType::Blue:    sample = generateBlueNoise(); break;
        case NoiseType::Violet:  sample = generateVioletNoise(); break;
        case NoiseType::Grey:    sample = generateGreyNoise(); break;
        case NoiseType::Crackle: sample = generateCrackleNoise(); break;
        case NoiseType::Digital: sample = generateDigitalNoise(); break;
        case NoiseType::Wind:    sample = generateWindNoise(); break;
        case NoiseType::Ocean:   sample = generateOceanNoise(); break;
    }

    return sample * level;
}

void FreOscNoiseGenerator::processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (level <= 0.0f)
        return;

    // Calculate pan gains for stereo positioning
    float leftGain = (pan <= 0.0f) ? 1.0f : (1.0f - pan);
    float rightGain = (pan >= 0.0f) ? 1.0f : (1.0f + pan);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float noiseSample = processSample();

        // Apply panning and add to buffer
        buffer.addSample(0, startSample + sample, noiseSample * leftGain);
        if (buffer.getNumChannels() > 1)
            buffer.addSample(1, startSample + sample, noiseSample * rightGain);
    }
}

//==============================================================================
// Noise generation implementations - matching JavaScript algorithms exactly

float FreOscNoiseGenerator::generateWhiteNoise()
{
    // White noise - equal power across all frequencies
    return random.nextFloat() * 2.0f - 1.0f;
}

float FreOscNoiseGenerator::generatePinkNoise()
{
    // Pink noise - 1/f frequency response using pole-zero filters
    // Algorithm from JavaScript implementation
    float white = random.nextFloat() * 2.0f - 1.0f;

    pinkB0 = 0.99886f * pinkB0 + white * 0.0555179f;
    pinkB1 = 0.99332f * pinkB1 + white * 0.0750759f;
    pinkB2 = 0.96900f * pinkB2 + white * 0.1538520f;
    pinkB3 = 0.86650f * pinkB3 + white * 0.3104856f;
    pinkB4 = 0.55000f * pinkB4 + white * 0.5329522f;
    pinkB5 = -0.7616f * pinkB5 - white * 0.0168980f;

    float pink = (pinkB0 + pinkB1 + pinkB2 + pinkB3 + pinkB4 + pinkB5 + pinkB6 + white * 0.5362f) * 0.11f;
    pinkB6 = white * 0.115926f;

    return pink;
}

float FreOscNoiseGenerator::generateBrownNoise()
{
    // Brown noise - 1/f^2 frequency response (Brownian noise)
    // Integration of white noise
    float white = random.nextFloat() * 2.0f - 1.0f;
    brownState = (brownState + (0.02f * white)) / 1.02f;
    return brownState * 3.5f; // Compensate for volume reduction
}

float FreOscNoiseGenerator::generateBlueNoise()
{
    // Blue noise - f frequency response (opposite of pink)
    // Differentiation of white noise
    float white = random.nextFloat() * 2.0f - 1.0f;
    float blue = white - blueLastOut;
    blueLastOut = white;
    return blue * 0.5f; // Compensate for increased amplitude
}

float FreOscNoiseGenerator::generateVioletNoise()
{
    // Violet noise - f^2 frequency response (opposite of brown)
    // Double differentiation
    float white = random.nextFloat() * 2.0f - 1.0f;
    float violet = white - 2.0f * violetLastOut + violetLastOut2;
    violetLastOut2 = violetLastOut;
    violetLastOut = white;
    return violet * 0.25f; // Compensate for increased amplitude
}

float FreOscNoiseGenerator::generateGreyNoise()
{
    // Grey noise - psychoacoustically flat noise
    // Complex filter network to match human hearing sensitivity
    float white = random.nextFloat() * 2.0f - 1.0f;

    greyB0 = 0.99765f * greyB0 + white * 0.0990460f;
    greyB1 = 0.96300f * greyB1 + white * 0.2965164f;
    greyB2 = 0.57000f * greyB2 + white * 1.0526913f;
    greyB3 = 0.14001f * greyB3 + white * 0.1848f;

    return (greyB0 + greyB1 + greyB2 + greyB3 + white * 0.0362f) * 0.15f;
}

float FreOscNoiseGenerator::generateCrackleNoise()
{
    // Vinyl crackle - sparse random pops and clicks
    float crackle = 0.0f;

    if (random.nextFloat() < 0.002f) // Sparse pops (0.2% chance per sample)
    {
        crackle = (random.nextFloat() * 2.0f - 1.0f) * random.nextFloat(); // Random amplitude pop
    }
    else if (random.nextFloat() < 0.01f) // Background hiss (1% chance per sample)
    {
        crackle = (random.nextFloat() * 2.0f - 1.0f) * 0.1f;
    }

    return crackle;
}

float FreOscNoiseGenerator::generateDigitalNoise()
{
    // Digital noise - quantized/aliased noise
    float white = random.nextFloat() * 2.0f - 1.0f;

    // Quantize to simulate low bit depth
    float sample = std::floor(white * 32.0f) / 32.0f;

    // Add some aliasing by mixing with previous sample
    sample += digitalLastSample * 0.3f;
    digitalLastSample = sample;

    // Clamp to prevent overflow
    return juce::jlimit(-1.0f, 1.0f, sample);
}

float FreOscNoiseGenerator::generateWindNoise()
{
    // Wind noise - low frequency rumble with variations
    float white = random.nextFloat() * 2.0f - 1.0f;

    // Heavy low-pass filtering for rumble
    windB0 = 0.999f * windB0 + white * 0.001f;
    windB1 = 0.995f * windB1 + windB0 * 0.005f;
    windB2 = 0.99f * windB2 + windB1 * 0.01f;

    // Add slow modulation
    windModPhase += 0.0001f;
    if (windModPhase > juce::MathConstants<float>::twoPi)
        windModPhase -= juce::MathConstants<float>::twoPi;

    float mod = std::sin(windModPhase) * 0.3f;

    return windB2 * (1.0f + mod) * 8.0f; // Amplify the quiet result
}

float FreOscNoiseGenerator::generateOceanNoise()
{
    // Ocean waves - filtered noise with wave-like modulation
    float white = random.nextFloat() * 2.0f - 1.0f;

    // Band-pass filtering for wave sound
    oceanB0 = 0.995f * oceanB0 + white * 0.005f;
    oceanB1 = 0.98f * oceanB1 + (oceanB0 - oceanB2) * 0.02f;
    oceanB2 = 0.99f * oceanB2 + oceanB1 * 0.01f;

    // Wave modulation - slow and irregular
    oceanWavePhase += 0.00005f + random.nextFloat() * 0.00002f;
    if (oceanWavePhase > juce::MathConstants<float>::twoPi)
        oceanWavePhase -= juce::MathConstants<float>::twoPi;

    oceanWaveAmp = std::sin(oceanWavePhase) + std::sin(oceanWavePhase * 2.3f) * 0.5f;
    oceanWaveAmp = juce::jmax(0.0f, oceanWaveAmp); // Only positive waves

    return oceanB1 * (0.3f + oceanWaveAmp * 0.7f) * 3.0f;
}