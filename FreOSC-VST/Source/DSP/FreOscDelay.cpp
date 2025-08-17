#include "FreOscDelay.h"

//==============================================================================
FreOscDelay::FreOscDelay()
{
}

FreOscDelay::~FreOscDelay()
{
}

//==============================================================================
void FreOscDelay::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // Prepare delay line with 1 second maximum delay
    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples(static_cast<int>(sampleRate * 1.0)); // 1 second max

    // Prepare gain stages
    wetGain.prepare(spec);
    dryGain.prepare(spec);
    feedbackGain.prepare(spec);

    // Initialize parameter smoothing
    smoothedDelayTime.reset(sampleRate, 0.05); // 50ms ramp time for smooth changes
    smoothedDelayTime.setCurrentAndTargetValue(currentDelayTime);

    // Set initial parameters
    updateDelayParameters();
    updateMixLevels();
}

void FreOscDelay::reset()
{
    delayLine.reset();
    wetGain.reset();
    dryGain.reset();
    feedbackGain.reset();
    smoothedDelayTime.reset(sampleRate, 0.05);
}

void FreOscDelay::process(const juce::dsp::ProcessContextReplacing<float>& context)
{
    auto& inputBlock = context.getInputBlock();
    auto& outputBlock = context.getOutputBlock();

    // Ensure we have enough channels
    jassert(inputBlock.getNumChannels() == outputBlock.getNumChannels());
    jassert(inputBlock.getNumSamples() == outputBlock.getNumSamples());

    // Process each sample
    for (size_t sample = 0; sample < inputBlock.getNumSamples(); ++sample)
    {
        // Update delay time smoothly
        float currentDelaySamples = smoothedDelayTime.getNextValue() * static_cast<float>(sampleRate) / 1000.0f;
        delayLine.setDelay(currentDelaySamples);

        // Process each channel
        for (size_t channel = 0; channel < inputBlock.getNumChannels(); ++channel)
        {
            // Get input sample
            float inputSample = inputBlock.getSample(static_cast<int>(channel), static_cast<int>(sample));

            // Read delayed sample
            float delayedSample = delayLine.popSample(static_cast<int>(channel));

            // Create feedback signal (with limiting to prevent runaway feedback)
            float feedbackSample = delayedSample * currentFeedback;

            // Sum input with feedback for delay line input
            float delayInput = inputSample + feedbackSample;

            // Push new sample into delay line
            delayLine.pushSample(static_cast<int>(channel), delayInput);

            // Mix dry and wet signals
            float drySignal = inputSample * (1.0f - currentWetLevel);
            float wetSignal = delayedSample * currentWetLevel;

            // Output mixed signal
            outputBlock.setSample(static_cast<int>(channel), static_cast<int>(sample),
                                drySignal + wetSignal);
        }
    }
}

//==============================================================================
void FreOscDelay::setDelayTime(float delayTimeMs)
{
    // Limit delay time to valid range (10ms to 1000ms)
    float newDelayTime = juce::jlimit(10.0f, 1000.0f, delayTimeMs);

    if (std::abs(currentDelayTime - newDelayTime) > 0.1f)
    {
        currentDelayTime = newDelayTime;
        smoothedDelayTime.setTargetValue(currentDelayTime);
    }
}

void FreOscDelay::setFeedback(float feedback)
{
    // Limit feedback to prevent runaway (max 95%)
    float newFeedback = juce::jlimit(0.0f, 0.95f, feedback);

    if (std::abs(currentFeedback - newFeedback) > 0.001f)
    {
        currentFeedback = newFeedback;
        updateDelayParameters();
    }
}

void FreOscDelay::setWetLevel(float wetLevel)
{
    float newWetLevel = juce::jlimit(0.0f, 1.0f, wetLevel);

    if (std::abs(currentWetLevel - newWetLevel) > 0.001f)
    {
        currentWetLevel = newWetLevel;
        updateMixLevels();
    }
}

//==============================================================================
void FreOscDelay::updateDelayParameters()
{
    // Set feedback gain (already limited in setter)
    feedbackGain.setGainLinear(currentFeedback);
}

void FreOscDelay::updateMixLevels()
{
    // Update wet/dry mix levels
    // Note: wet/dry mixing is handled directly in process() for better control
    wetGain.setGainLinear(currentWetLevel);
    dryGain.setGainLinear(1.0f - currentWetLevel);
}