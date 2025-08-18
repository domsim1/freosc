#include "FreOscPlateReverb.h"

//==============================================================================
FreOscPlateReverb::FreOscPlateReverb()
{
}

FreOscPlateReverb::~FreOscPlateReverb()
{
}

//==============================================================================
void FreOscPlateReverb::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    
    // Prepare pre-delay (max 250ms)
    int maxPreDelaySamples = static_cast<int>(sampleRate * 0.25);
    preDelayLine.setSize(maxPreDelaySamples);
    
    // Initialize allpass filters with base delays scaled for sample rate
    double sampleRateRatio = sampleRate / 44100.0;
    for (int i = 0; i < numAllpass; ++i)
    {
        int delaySize = static_cast<int>(baseAllpassDelays[i] * sampleRateRatio);
        allpass[i].setDelay(delaySize);
    }
    
    // Initialize comb filters 
    for (int i = 0; i < numCombs / 2; ++i)
    {
        int leftDelaySize = static_cast<int>(baseCombDelays[i] * sampleRateRatio);
        int rightDelaySize = static_cast<int>(baseCombDelays[i + numCombs / 2] * sampleRateRatio);
        
        combsL[i].setDelay(leftDelaySize);
        combsR[i].setDelay(rightDelaySize);
    }
    
    // Prepare damping filters (lowpass for HF rolloff)
    dampingFilterL.prepare(spec);
    dampingFilterR.prepare(spec);
    
    // Initialize stereo processing coefficients
    updateStereoMatrix();
    
    // Prepare gain stages
    wetGain.prepare(spec);
    dryGain.prepare(spec);
    
    // Update all parameters
    updateDelayTimes();
    updateFeedback(); 
    updateDamping();
    updateStereoMatrix();
    updateMixLevels();
}

void FreOscPlateReverb::reset()
{
    preDelayLine.clear();
    
    for (int i = 0; i < numAllpass; ++i)
    {
        allpass[i].delay.clear();
    }
    
    for (int i = 0; i < numCombs / 2; ++i)
    {
        combsL[i].delay.clear();
        combsL[i].lastOutput = 0.0f;
        combsR[i].delay.clear();
        combsR[i].lastOutput = 0.0f;
    }
    
    dampingFilterL.reset();
    dampingFilterR.reset();
    wetGain.reset();
    dryGain.reset();
}

void FreOscPlateReverb::process(const juce::dsp::ProcessContextReplacing<float>& context)
{
    // Early exit if wet level is zero (bypass)
    if (currentWetLevel <= 0.001f)
        return;
        
    auto& inputBlock = context.getInputBlock();
    auto& outputBlock = context.getOutputBlock();
    
    auto numChannels = static_cast<int>(inputBlock.getNumChannels());
    auto numSamples = static_cast<int>(inputBlock.getNumSamples());
    
    // Process each sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Get input (mono sum for stereo input)
        float input = inputBlock.getSample(0, sample);
        if (numChannels > 1)
            input = (input + inputBlock.getSample(1, sample)) * 0.5f;
            
        // Store dry signal
        float dryL = inputBlock.getSample(0, sample);
        float dryR = (numChannels > 1) ? inputBlock.getSample(1, sample) : dryL;
        
        // Apply pre-delay
        float delayedInput = preDelayLine.readInterpolated(currentPreDelay * preDelayLine.size);
        preDelayLine.write(input);
        
        // Process through allpass diffusers
        float diffused = delayedInput;
        for (int i = 0; i < numAllpass; ++i)
        {
            diffused = allpass[i].process(diffused);
        }
        
        // Process through parallel comb filters with proper scaling
        float reverbL = 0.0f;
        float reverbR = 0.0f;
        
        for (int i = 0; i < numCombs / 2; ++i)
        {
            reverbL += combsL[i].process(diffused);
            reverbR += combsR[i].process(diffused);
        }
        
        // Scale comb filter output for lush but controlled levels
        float combScale = 1.0f / static_cast<float>(numCombs / 2); // Normalize by number of combs
        combScale *= 0.6f; // Less aggressive attenuation for more lush sound
        reverbL *= combScale;
        reverbR *= combScale;
        
        // Apply high frequency damping
        reverbL = dampingFilterL.processSample(reverbL);
        reverbR = dampingFilterR.processSample(reverbR);
        
        // Apply stereo width processing (simpler approach)
        // Width = 0: mono (L+R)/2, Width = 1: full stereo
        float mono = (reverbL + reverbR) * 0.5f;
        float widthAmount = currentStereoWidth;
        reverbL = mono + (reverbL - mono) * widthAmount;
        reverbR = mono + (reverbR - mono) * widthAmount;
        
        // Mix wet and dry signals
        float outputL = dryGain.processSample(dryL) + wetGain.processSample(reverbL);
        float outputR = dryGain.processSample(dryR) + wetGain.processSample(reverbR);
        
        // Soft limiting to prevent any remaining peaks
        outputL = juce::jlimit(-1.5f, 1.5f, outputL);
        outputR = juce::jlimit(-1.5f, 1.5f, outputR);
        
        // Write output
        outputBlock.setSample(0, sample, outputL);
        if (numChannels > 1)
            outputBlock.setSample(1, sample, outputR);
    }
}

//==============================================================================
void FreOscPlateReverb::setPreDelay(float preDelay)
{
    currentPreDelay = juce::jlimit(0.0f, 1.0f, preDelay);
}

void FreOscPlateReverb::setSize(float size)
{
    currentSize = juce::jlimit(0.0f, 1.0f, size);
    updateDelayTimes();
    updateFeedback();
}

void FreOscPlateReverb::setDamping(float damping)
{
    currentDamping = juce::jlimit(0.0f, 1.0f, damping);
    updateDamping();
}

void FreOscPlateReverb::setDiffusion(float diffusion)
{
    currentDiffusion = juce::jlimit(0.0f, 1.0f, diffusion);
    
    // Update allpass gain based on diffusion - tamed gains to prevent peaks
    float gain = 0.4f + currentDiffusion * 0.3f; // 0.4 to 0.7 (lush but controlled)
    for (int i = 0; i < numAllpass; ++i)
    {
        allpass[i].gain = gain;
    }
}

void FreOscPlateReverb::setWetLevel(float wetLevel)
{
    currentWetLevel = juce::jlimit(0.0f, 1.0f, wetLevel);
    updateMixLevels();
}

void FreOscPlateReverb::setStereoWidth(float width)
{
    currentStereoWidth = juce::jlimit(0.0f, 1.0f, width);
    updateStereoMatrix();
}

//==============================================================================
void FreOscPlateReverb::updateDelayTimes()
{
    // Size affects delay times slightly for different decay characteristics
    double sizeMultiplier = 0.6 + currentSize * 0.8; // 0.6 to 1.4 multiplier for longer tails
    double sampleRateRatio = sampleRate / 44100.0;
    
    for (int i = 0; i < numCombs / 2; ++i)
    {
        int leftDelaySize = static_cast<int>(baseCombDelays[i] * sampleRateRatio * sizeMultiplier);
        int rightDelaySize = static_cast<int>(baseCombDelays[i + numCombs / 2] * sampleRateRatio * sizeMultiplier);
        
        combsL[i].setDelay(leftDelaySize);
        combsR[i].setDelay(rightDelaySize);
    }
}

void FreOscPlateReverb::updateFeedback()
{
    // Size controls decay time via feedback - balanced for lush but stable sound
    float feedback = 0.15f + currentSize * 0.6f; // 0.15 to 0.75 feedback (more lush while stable)
    
    for (int i = 0; i < numCombs / 2; ++i)
    {
        combsL[i].setFeedback(feedback);
        combsR[i].setFeedback(feedback);
    }
}

void FreOscPlateReverb::updateDamping()
{
    // Damping controls high frequency rolloff
    float cutoffFreq = 2000.0f + (1.0f - currentDamping) * 8000.0f; // 2kHz to 10kHz
    
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutoffFreq, 0.7f);
    dampingFilterL.coefficients = coeffs;
    dampingFilterR.coefficients = coeffs;
}

void FreOscPlateReverb::updateStereoMatrix()
{
    // Stereo width processing is now handled inline in the process method
    // This method exists for API compatibility but doesn't need to do anything
}

void FreOscPlateReverb::updateMixLevels()
{
    // Equal power crossfade
    float wet = currentWetLevel;
    float dry = std::sqrt(1.0f - wet * wet);
    wet = std::sqrt(wet);
    
    wetGain.setGainLinear(wet);
    dryGain.setGainLinear(dry);
}

//==============================================================================
// DelayLine implementation
void FreOscPlateReverb::DelayLine::setSize(int newSize)
{
    if (newSize != size)
    {
        size = newSize;
        buffer.resize(size);
        clear();
    }
}

void FreOscPlateReverb::DelayLine::clear()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writePos = 0;
}

float FreOscPlateReverb::DelayLine::read(int delaySamples) const
{
    if (size == 0) return 0.0f;
    
    int readPos = (writePos - delaySamples + size) % size;
    return buffer[readPos];
}

void FreOscPlateReverb::DelayLine::write(float sample)
{
    if (size > 0)
    {
        buffer[writePos] = sample;
        writePos = (writePos + 1) % size;
    }
}

float FreOscPlateReverb::DelayLine::readInterpolated(float delaySamples) const
{
    if (size == 0) return 0.0f;
    
    int delay1 = static_cast<int>(delaySamples);
    int delay2 = delay1 + 1;
    float frac = delaySamples - delay1;
    
    float sample1 = read(delay1);
    float sample2 = read(delay2);
    
    return sample1 + frac * (sample2 - sample1);
}

//==============================================================================
// AllpassFilter implementation
void FreOscPlateReverb::AllpassFilter::setDelay(int samples)
{
    delay.setSize(samples);
}

float FreOscPlateReverb::AllpassFilter::process(float input)
{
    float delayedSignal = delay.read(delay.size - 1);
    float output = -gain * input + delayedSignal;
    delay.write(input + gain * delayedSignal);
    return output;
}

//==============================================================================
// CombFilter implementation  
void FreOscPlateReverb::CombFilter::setDelay(int samples)
{
    delay.setSize(samples);
}

void FreOscPlateReverb::CombFilter::setFeedback(float fb)
{
    feedback = juce::jlimit(0.0f, 0.95f, fb);
}

void FreOscPlateReverb::CombFilter::setDamping(float damp)
{
    dampingGain = juce::jlimit(0.0f, 1.0f, damp);
}

float FreOscPlateReverb::CombFilter::process(float input)
{
    float delayedSignal = delay.read(delay.size - 1);
    
    // Apply damping (simple lowpass)
    lastOutput = delayedSignal * (1.0f - dampingGain) + lastOutput * dampingGain;
    
    float output = input + feedback * lastOutput;
    delay.write(output);
    
    return delayedSignal;
}