#include "FreOscLimiter.h"

//==============================================================================
FreOscLimiter::FreOscLimiter()
{
    // Soft saturation will be handled manually in processStereoSample
}

FreOscLimiter::~FreOscLimiter()
{
}

//==============================================================================
void FreOscLimiter::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    
    // Setup lookahead buffer
    lookaheadSamples = static_cast<int>(lookaheadTime * sampleRate / 1000.0f);
    lookaheadSamples = juce::jmin(lookaheadSamples, maxLookaheadSamples);
    lookaheadBuffer.setSize(static_cast<int>(spec.numChannels), lookaheadSamples + 1);
    
    // Setup gain reduction smoother
    gainReductionSmooth.reset(sampleRate, releaseTime / 1000.0);
    
    // Soft saturation handled manually - no need to prepare softClipper
    
    // Setup DC blockers (20Hz high-pass)
    auto dcCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
    dcBlockerL.coefficients = dcCoeffs;
    dcBlockerR.coefficients = dcCoeffs;
    
    reset();
}

void FreOscLimiter::reset()
{
    lookaheadBuffer.clear();
    bufferWritePos = 0;
    
    gainReductionSmooth.setCurrentAndTargetValue(0.0f);
    peakHoldLevel = 0.0f;
    peakHoldCounter = 0;
    
    // Soft saturation reset handled in other state variables
    dcBlockerL.reset();
    dcBlockerR.reset();
    
    currentGainReduction = 0.0f;
    currentInputLevel = 0.0f;
    currentOutputLevel = 0.0f;
}

//==============================================================================
std::pair<float, float> FreOscLimiter::processStereoSample(float inputL, float inputR)
{
    // Store input level for metering
    float inputMagnitude = juce::jmax(std::abs(inputL), std::abs(inputR));
    currentInputLevel = linearToDb(inputMagnitude);
    
    // Write to lookahead buffer
    lookaheadBuffer.setSample(0, bufferWritePos, inputL);
    if (lookaheadBuffer.getNumChannels() > 1)
        lookaheadBuffer.setSample(1, bufferWritePos, inputR);
    
    // Get delayed samples for processing
    int readPos = (bufferWritePos - lookaheadSamples + lookaheadBuffer.getNumSamples()) % lookaheadBuffer.getNumSamples();
    float delayedL = lookaheadBuffer.getSample(0, readPos);
    float delayedR = lookaheadBuffer.getNumChannels() > 1 ? 
                     lookaheadBuffer.getSample(1, readPos) : delayedL;
    
    // Advance buffer position
    bufferWritePos = (bufferWritePos + 1) % lookaheadBuffer.getNumSamples();
    
    // Peak detection on current input (lookahead)
    float currentPeak = detectPeak(inputMagnitude);
    
    // Calculate required gain reduction
    float targetGainReduction = calculateGainReduction(currentPeak);
    
    // Smooth the gain reduction 
    if (targetGainReduction > gainReductionSmooth.getCurrentValue())
    {
        // Attack - instant for limiting
        gainReductionSmooth.setCurrentAndTargetValue(targetGainReduction);
    }
    else
    {
        // Release - smooth
        gainReductionSmooth.reset(sampleRate, releaseTime / 1000.0);
        gainReductionSmooth.setTargetValue(targetGainReduction);
    }
    
    float smoothedGainReduction = gainReductionSmooth.getNextValue();
    currentGainReduction = smoothedGainReduction;
    
    // Apply gain reduction to delayed samples
    float gainMultiplier = dbToLinear(-smoothedGainReduction);
    float limitedL = delayedL * gainMultiplier;
    float limitedR = delayedR * gainMultiplier;
    
    // Apply soft saturation for musical character
    limitedL = softSaturate(limitedL, saturationAmount);
    limitedR = softSaturate(limitedR, saturationAmount);
    
    // Final hard ceiling (safety limiter)
    float ceilingMultiplier = dbToLinear(ceiling);
    limitedL = juce::jlimit(-ceilingMultiplier, ceilingMultiplier, limitedL);
    limitedR = juce::jlimit(-ceilingMultiplier, ceilingMultiplier, limitedR);
    
    // Remove any DC offset from saturation
    limitedL = dcBlockerL.processSample(limitedL);
    limitedR = dcBlockerR.processSample(limitedR);
    
    // Store output level for metering
    float outputMagnitude = juce::jmax(std::abs(limitedL), std::abs(limitedR));
    currentOutputLevel = linearToDb(outputMagnitude);
    
    return {limitedL, limitedR};
}

float FreOscLimiter::detectPeak(float sample)
{
    float absSample = std::abs(sample);
    
    // Peak hold with decay
    if (absSample > peakHoldLevel)
    {
        peakHoldLevel = absSample;
        peakHoldCounter = peakHoldTime;
    }
    else if (--peakHoldCounter <= 0)
    {
        peakHoldLevel *= 0.999f; // Slow decay
    }
    
    return peakHoldLevel;
}

float FreOscLimiter::calculateGainReduction(float peakLevel)
{
    float peakDb = linearToDb(peakLevel);
    
    if (peakDb <= threshold)
        return 0.0f; // No limiting needed
    
    // Calculate overshoot above threshold
    float overshoot = peakDb - threshold;
    return overshoot; // 1:∞ ratio for true limiting
}

float FreOscLimiter::softSaturate(float input, float amount)
{
    if (amount <= 0.0f)
        return input;
    
    // Blend between clean and saturated signal
    float saturated = tanhSaturation(input * (1.0f + amount * 2.0f)) / (1.0f + amount * 0.5f);
    return input * (1.0f - amount) + saturated * amount;
}

//==============================================================================
void FreOscLimiter::setThreshold(float thresholdDb)
{
    threshold = juce::jlimit(-20.0f, 0.0f, thresholdDb);
}

void FreOscLimiter::setRelease(float releaseMs)
{
    releaseTime = juce::jlimit(1.0f, 1000.0f, releaseMs);
}

void FreOscLimiter::setCeiling(float ceilingDb)
{
    ceiling = juce::jlimit(-1.0f, 0.0f, ceilingDb);
}

void FreOscLimiter::setSaturation(float amount)
{
    saturationAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void FreOscLimiter::setLookahead(float lookaheadMs)
{
    lookaheadTime = juce::jlimit(0.0f, 10.0f, lookaheadMs);
    // Note: Lookahead change requires re-preparing the processor
}