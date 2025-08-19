#include "FreOscCompressor.h"

//==============================================================================
FreOscCompressor::FreOscCompressor()
{
}

FreOscCompressor::~FreOscCompressor()
{
}

//==============================================================================
void FreOscCompressor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    
    // Setup smooth value objects for musical attack/release
    rmsLevel.reset(sampleRate, 0.001); // 1ms for RMS smoothing
    gainReductionSmooth.reset(sampleRate, attackTime / 1000.0); // Attack time in seconds
    makeupGainSmooth.reset(sampleRate, 0.05); // 50ms for makeup gain changes
    
    // Setup sidechain high-pass filter (80Hz) to reduce pumping on bass
    auto coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 80.0f);
    sidechainHPF_L.coefficients = coefficients;
    sidechainHPF_R.coefficients = coefficients;
    
    reset();
}

void FreOscCompressor::reset()
{
    rmsLevel.setCurrentAndTargetValue(0.0f);
    gainReductionSmooth.setCurrentAndTargetValue(0.0f);
    makeupGainSmooth.setCurrentAndTargetValue(dbToLinear(makeupGain));
    
    sidechainHPF_L.reset();
    sidechainHPF_R.reset();
    
    currentGainReduction = 0.0f;
    currentInputLevel = 0.0f;
    currentOutputLevel = 0.0f;
}

//==============================================================================
std::pair<float, float> FreOscCompressor::processStereoSample(float inputL, float inputR)
{
    // Store input level for metering
    float inputMagnitude = std::sqrt((inputL * inputL + inputR * inputR) / 2.0f);
    currentInputLevel = linearToDb(inputMagnitude);
    
    // Apply sidechain high-pass filtering for detection
    float sidechainL = sidechainHPF_L.processSample(inputL);
    float sidechainR = sidechainHPF_R.processSample(inputR);
    
    // Calculate RMS level from sidechain signal
    float sidechainMagnitude = std::sqrt((sidechainL * sidechainL + sidechainR * sidechainR) / 2.0f);
    rmsLevel.setTargetValue(sidechainMagnitude);
    float currentRMS = rmsLevel.getNextValue();
    float rmsDb = linearToDb(currentRMS);
    
    // Calculate required gain reduction
    float targetGainReduction = calculateGainReduction(rmsDb);
    
    // Smooth the gain reduction with attack/release
    if (targetGainReduction > gainReductionSmooth.getCurrentValue())
    {
        // Attack phase - fast response
        gainReductionSmooth.reset(sampleRate, attackTime / 1000.0);
    }
    else
    {
        // Release phase - slower response
        gainReductionSmooth.reset(sampleRate, releaseTime / 1000.0);
    }
    
    gainReductionSmooth.setTargetValue(targetGainReduction);
    float smoothedGainReduction = gainReductionSmooth.getNextValue();
    
    // Store for metering
    currentGainReduction = smoothedGainReduction;
    
    // Convert gain reduction to linear multiplier
    float gainMultiplier = dbToLinear(-smoothedGainReduction);
    
    // Apply makeup gain
    float makeupMultiplier = makeupGainSmooth.getNextValue();
    
    // Apply compression and makeup gain
    float compressedL = inputL * gainMultiplier * makeupMultiplier;
    float compressedR = inputR * gainMultiplier * makeupMultiplier;
    
    // Apply wet/dry mix
    float outputL = inputL * (1.0f - mixAmount) + compressedL * mixAmount;
    float outputR = inputR * (1.0f - mixAmount) + compressedR * mixAmount;
    
    // Store output level for metering
    float outputMagnitude = std::sqrt((outputL * outputL + outputR * outputR) / 2.0f);
    currentOutputLevel = linearToDb(outputMagnitude);
    
    return {outputL, outputR};
}

float FreOscCompressor::calculateGainReduction(float inputLevelDb)
{
    if (inputLevelDb <= threshold)
        return 0.0f; // No compression below threshold
    
    // Use soft-knee compression for smooth transition
    return softKneeCompression(inputLevelDb, threshold, ratio, kneeWidth);
}

float FreOscCompressor::softKneeCompression(float inputLevel, float thresholdDb, float ratioValue, float kneeDb)
{
    float kneeStart = thresholdDb - kneeDb / 2.0f;
    float kneeEnd = thresholdDb + kneeDb / 2.0f;
    
    if (inputLevel <= kneeStart)
    {
        // Below soft knee - no compression
        return 0.0f;
    }
    else if (inputLevel >= kneeEnd)
    {
        // Above soft knee - full compression
        float overshoot = inputLevel - thresholdDb;
        return overshoot * (1.0f - 1.0f / ratioValue);
    }
    else
    {
        // In soft knee region - smooth transition
        float kneeRatio = (inputLevel - kneeStart) / kneeDb;
        float softRatio = 1.0f + (ratioValue - 1.0f) * kneeRatio * kneeRatio; // Smooth curve
        float overshoot = inputLevel - thresholdDb;
        return overshoot * (1.0f - 1.0f / softRatio);
    }
}

//==============================================================================
void FreOscCompressor::setThreshold(float thresholdDb)
{
    threshold = juce::jlimit(-60.0f, 0.0f, thresholdDb);
}

void FreOscCompressor::setRatio(float ratioValue)
{
    ratio = juce::jlimit(1.0f, 20.0f, ratioValue);
}

void FreOscCompressor::setAttack(float attackMs)
{
    attackTime = juce::jlimit(0.1f, 100.0f, attackMs);
}

void FreOscCompressor::setRelease(float releaseMs)
{
    releaseTime = juce::jlimit(10.0f, 1000.0f, releaseMs);
}

void FreOscCompressor::setKnee(float kneeDb)
{
    kneeWidth = juce::jlimit(0.0f, 10.0f, kneeDb);
}

void FreOscCompressor::setMakeupGain(float gainDb)
{
    makeupGain = juce::jlimit(-20.0f, 20.0f, gainDb);
    makeupGainSmooth.setTargetValue(dbToLinear(makeupGain));
}

void FreOscCompressor::setMix(float mix)
{
    mixAmount = juce::jlimit(0.0f, 1.0f, mix);
}