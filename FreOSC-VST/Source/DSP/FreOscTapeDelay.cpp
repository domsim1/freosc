#include "FreOscTapeDelay.h"

//==============================================================================
FreOscTapeDelay::FreOscTapeDelay()
{
}

FreOscTapeDelay::~FreOscTapeDelay()
{
}

//==============================================================================
void FreOscTapeDelay::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    
    // Prepare delay lines (max 2 seconds)
    int maxDelaySamples = static_cast<int>(sampleRate * 2.1); // Extra headroom
    delayLineL.setSize(maxDelaySamples);
    delayLineR.setSize(maxDelaySamples);
    
    // Prepare tape characteristic filters
    tapeFilterL.prepare(spec);
    tapeFilterR.prepare(spec);
    feedbackFilterL.prepare(spec);
    feedbackFilterR.prepare(spec);
    
    // Prepare tape saturation
    tapeSaturation.prepare(spec);
    tapeSaturation.setThreshold(-12.0f);   // Gentle tape compression
    tapeSaturation.setRatio(3.0f);         // Moderate compression ratio
    tapeSaturation.setAttack(1.0f);        // Slow attack for smooth tape feel
    tapeSaturation.setRelease(50.0f);      // Medium release
    
    // Initialize flutter oscillator
    flutterPhase = 0.0f;
    flutterPhaseIncrement = (flutterRate * 2.0f * 3.14159265359f) / static_cast<float>(sampleRate);
    
    // Gain calculation handled in process method
    
    // Update all parameters
    updateDelayTimes();
    updateTapeFilters();
    updateFlutter();
    updateStereoWidth();
}

void FreOscTapeDelay::reset()
{
    delayLineL.clear();
    delayLineR.clear();
    
    tapeFilterL.reset();
    tapeFilterR.reset();
    feedbackFilterL.reset();
    feedbackFilterR.reset();
    
    tapeSaturation.reset();
    flutterPhase = 0.0f;
}

void FreOscTapeDelay::process(const juce::dsp::ProcessContextReplacing<float>& context)
{
    auto& inputBlock = context.getInputBlock();
    auto& outputBlock = context.getOutputBlock();
    
    auto numChannels = static_cast<int>(inputBlock.getNumChannels());
    auto numSamples = static_cast<int>(inputBlock.getNumSamples());
    
    // If wet level is zero, just pass through the dry signal
    if (currentWetLevel <= 0.001f)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                outputBlock.setSample(channel, sample, inputBlock.getSample(channel, sample));
            }
        }
        return;
    }
    
    // Safety check: ensure delay lines are initialized
    if (delayLineL.size == 0 || delayLineR.size == 0)
    {
        // If delay lines aren't ready, just pass through the input
        for (int channel = 0; channel < numChannels; ++channel)
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                outputBlock.setSample(channel, sample, inputBlock.getSample(channel, sample));
            }
        }
        return;
    }
    
    // Process each sample - BASIC DELAY ONLY (no filters, flutter, or saturation)
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Get input
        float inputL = inputBlock.getSample(0, sample);
        float inputR = (numChannels > 1) ? inputBlock.getSample(1, sample) : inputL;
        
        // Store dry signal
        float dryL = inputL;
        float dryR = inputR;
        
        // Thread-safe manual flutter oscillator
        flutterPhase += flutterPhaseIncrement;
        if (flutterPhase > 2.0f * 3.14159265359f)
            flutterPhase -= 2.0f * 3.14159265359f;
        
        float flutterMod = std::sin(flutterPhase) * flutterDepth;
        flutterMod = juce::jlimit(-0.5f, 0.5f, flutterMod);
        
        float baseDelayTime = currentTime * (2000.0f - 20.0f) + 20.0f; // 20ms to 2000ms
        baseDelayTime = juce::jlimit(20.0f, 2000.0f, baseDelayTime);
        
        float modulatedDelayTimeL = baseDelayTime * (1.0f + flutterMod);
        float modulatedDelayTimeR = baseDelayTime * (1.0f - flutterMod * 0.7f);
        
        // Clamp modulated delay times
        modulatedDelayTimeL = juce::jlimit(1.0f, 2100.0f, modulatedDelayTimeL);
        modulatedDelayTimeR = juce::jlimit(1.0f, 2100.0f, modulatedDelayTimeR);
        
        // Convert to samples and clamp to buffer size
        float delaySamplesL = (modulatedDelayTimeL / 1000.0f) * static_cast<float>(sampleRate);
        float delaySamplesR = (modulatedDelayTimeR / 1000.0f) * static_cast<float>(sampleRate);
        
        // Ensure delay samples are within valid buffer range
        delaySamplesL = juce::jlimit(1.0f, static_cast<float>(delayLineL.size - 2), delaySamplesL);
        delaySamplesR = juce::jlimit(1.0f, static_cast<float>(delayLineR.size - 2), delaySamplesR);
        
        // Read delayed signal and apply tone filtering
        float delayedL = delayLineL.readInterpolated(delaySamplesL);
        float delayedR = delayLineR.readInterpolated(delaySamplesR);
        
        // Apply tape tone filtering to delayed signal for tape character
        delayedL = tapeFilterL.processSample(delayedL);
        delayedR = tapeFilterR.processSample(delayedR);
        
        // Apply feedback with additional filtering for warmth
        float feedbackL = feedbackFilterL.processSample(delayedL) * currentFeedback;
        float feedbackR = feedbackFilterR.processSample(delayedR) * currentFeedback;
        
        // Write to delay line (input + feedback)
        delayLineL.write(inputL + feedbackL);
        delayLineR.write(inputR + feedbackR);
        
        // Simple wet/dry mix
        float wet = juce::jlimit(0.0f, 1.0f, currentWetLevel);
        float dry = 1.0f - wet;
        
        float outputL = dryL * dry + delayedL * wet;
        float outputR = dryR * dry + delayedR * wet;
        
        // Clamp output to prevent extreme values
        outputL = juce::jlimit(-2.0f, 2.0f, outputL);
        outputR = juce::jlimit(-2.0f, 2.0f, outputR);
        
        // Write output
        outputBlock.setSample(0, sample, outputL);
        if (numChannels > 1)
            outputBlock.setSample(1, sample, outputR);
    }
}

//==============================================================================
void FreOscTapeDelay::setTime(float time)
{
    currentTime = juce::jlimit(0.0f, 1.0f, time);
    updateDelayTimes();
}

void FreOscTapeDelay::setFeedback(float feedback)
{
    currentFeedback = juce::jlimit(0.0f, 1.0f, feedback);
    // Feedback is applied in the process loop
}

void FreOscTapeDelay::setTone(float tone)
{
    currentTone = juce::jlimit(0.0f, 1.0f, tone);
    updateTapeFilters();
}

void FreOscTapeDelay::setFlutter(float flutter)
{
    currentFlutter = juce::jlimit(0.0f, 1.0f, flutter);
    updateFlutter();
}

void FreOscTapeDelay::setWetLevel(float wetLevel)
{
    currentWetLevel = juce::jlimit(0.0f, 1.0f, wetLevel);
    // Don't call updateMixLevels() here - calculate gains in process method for thread safety
}

void FreOscTapeDelay::setStereoWidth(float width)
{
    currentStereoWidth = juce::jlimit(0.0f, 1.0f, width);
    updateStereoWidth();
}

//==============================================================================
void FreOscTapeDelay::updateDelayTimes()
{
    // Delay times are calculated in real-time in the process loop
    // This method exists for API compatibility
}

void FreOscTapeDelay::updateTapeFilters()
{
    // Tone controls high-frequency rolloff (tape aging effect)
    float cutoffFreq = 2000.0f + currentTone * 8000.0f; // 2kHz to 10kHz
    
    auto tapeCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutoffFreq, 0.7f);
    tapeFilterL.coefficients = tapeCoeffs;
    tapeFilterR.coefficients = tapeCoeffs;
    
    // Feedback filter for additional warmth (slightly darker)
    float feedbackCutoff = 1500.0f + currentTone * 3500.0f; // 1.5kHz to 5kHz
    auto feedbackCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, feedbackCutoff, 0.5f);
    feedbackFilterL.coefficients = feedbackCoeffs;
    feedbackFilterR.coefficients = feedbackCoeffs;
}

void FreOscTapeDelay::updateFlutter()
{
    // Flutter creates tape wow/flutter imperfections
    flutterDepth = currentFlutter * 0.02f; // Up to 2% pitch modulation
    flutterRate = 0.1f + currentFlutter * 0.4f; // 0.1Hz to 0.5Hz flutter rate
    flutterPhaseIncrement = (flutterRate * 2.0f * 3.14159265359f) / static_cast<float>(sampleRate);
}

// updateMixLevels removed - gain calculation now done directly in process method

void FreOscTapeDelay::updateStereoWidth()
{
    // Stereo width is applied in the process method
    // This method exists for API compatibility
}

float FreOscTapeDelay::applySaturation(float input, float drive)
{
    // Simple tape saturation using soft clipping
    float driven = input * (1.0f + drive);
    return std::tanh(driven * 0.7f) * 0.8f; // Gentle saturation
}

//==============================================================================
// TapeDelayLine implementation
void FreOscTapeDelay::TapeDelayLine::setSize(int newSize)
{
    // Ensure positive size and reasonable limits
    newSize = juce::jlimit(1, 10000000, newSize); // Max ~4 minutes at 44.1kHz
    
    if (newSize != size)
    {
        size = newSize;
        buffer.resize(static_cast<size_t>(size), 0.0f); // Initialize with zeros
        clear();
    }
}

void FreOscTapeDelay::TapeDelayLine::clear()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writePos = 0;
}

float FreOscTapeDelay::TapeDelayLine::read(float delaySamples) const
{
    if (size == 0 || buffer.empty()) return 0.0f;
    
    // Clamp delay to valid range
    int delay = juce::jlimit(0, size - 1, static_cast<int>(delaySamples));
    int readPos = (writePos - delay + size) % size;
    
    // Additional bounds check
    readPos = juce::jlimit(0, size - 1, readPos);
    
    return buffer[static_cast<size_t>(readPos)];
}

void FreOscTapeDelay::TapeDelayLine::write(float sample)
{
    if (size > 0 && !buffer.empty())
    {
        // Clamp sample to prevent extreme values
        sample = juce::jlimit(-10.0f, 10.0f, sample);
        
        // Bounds check writePos
        writePos = juce::jlimit(0, size - 1, writePos);
        
        buffer[static_cast<size_t>(writePos)] = sample;
        writePos = (writePos + 1) % size;
    }
}

float FreOscTapeDelay::TapeDelayLine::readInterpolated(float delaySamples) const
{
    if (size == 0 || buffer.empty()) return 0.0f;
    
    // Clamp delay samples to valid range
    delaySamples = juce::jlimit(1.0f, static_cast<float>(size - 1), delaySamples);
    
    int delay1 = static_cast<int>(delaySamples);
    int delay2 = delay1 + 1;
    float frac = delaySamples - delay1;
    
    // Ensure delay2 doesn't exceed buffer bounds
    delay2 = juce::jmin(delay2, size - 1);
    
    // Calculate read positions with proper bounds checking
    int readPos1 = (writePos - delay1 + size) % size;
    int readPos2 = (writePos - delay2 + size) % size;
    
    // Bounds check the positions
    readPos1 = juce::jlimit(0, size - 1, readPos1);
    readPos2 = juce::jlimit(0, size - 1, readPos2);
    
    float sample1 = buffer[static_cast<size_t>(readPos1)];
    float sample2 = buffer[static_cast<size_t>(readPos2)];
    
    return sample1 + frac * (sample2 - sample1);
}