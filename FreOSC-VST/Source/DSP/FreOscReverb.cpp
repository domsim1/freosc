#include "FreOscReverb.h"

//==============================================================================
FreOscReverb::FreOscReverb()
{
}

FreOscReverb::~FreOscReverb()
{
}

//==============================================================================
void FreOscReverb::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // Prepare convolution engine
    convolution.prepare(spec);

    // Prepare gain stages for wet/dry mixing
    wetGain.prepare(spec);
    dryGain.prepare(spec);

    // Prepare algorithmic reverb components
    for (int i = 0; i < 8; ++i)
    {
        delayLines[i].prepare(spec);
        delayLines[i].setMaximumDelayInSamples(static_cast<int>(sampleRate * 0.2f)); // 200ms max
        delayLines[i].setDelay(delayTimes[i] * static_cast<float>(sampleRate));
    }

    // Prepare damping filter (low-pass for high-frequency absorption)
    dampingFilter.prepare(spec);
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 8000.0f, 0.707f);
    dampingFilter.coefficients = coeffs;

    // Generate initial impulse response
    updateImpulseResponse();
    updateMixLevels();
}

void FreOscReverb::reset()
{
    convolution.reset();
    wetGain.reset();
    dryGain.reset();

    // Reset algorithmic reverb components
    for (int i = 0; i < 8; ++i)
    {
        delayLines[i].reset();
    }
    dampingFilter.reset();
}

void FreOscReverb::process(const juce::dsp::ProcessContextReplacing<float>& context)
{
    // Early exit if wet level is zero (bypass)
    if (currentWetLevel <= 0.001f)
        return;

    // Update impulse if parameters changed
    if (needsImpulseUpdate)
    {
        updateImpulseResponse();
        needsImpulseUpdate = false;
    }

    // Simple algorithmic reverb instead of complex convolution
    // This is more reliable and less CPU intensive
    processAlgorithmicReverb(context);
}

//==============================================================================
void FreOscReverb::setRoomSize(float roomSize)
{
    float newRoomSize = juce::jlimit(0.0f, 1.0f, roomSize);
    if (std::abs(currentRoomSize - newRoomSize) > 0.001f)
    {
        currentRoomSize = newRoomSize;
        needsImpulseUpdate = true;

        // Update delay line lengths based on room size
        for (int i = 0; i < 8; ++i)
        {
            float scaledDelay = delayTimes[i] * (0.7f + currentRoomSize * 0.6f); // 0.7x to 1.3x scaling
            delayLines[i].setDelay(scaledDelay * static_cast<float>(sampleRate));
        }
    }
}

void FreOscReverb::setWetLevel(float wetLevel)
{
    float newWetLevel = juce::jlimit(0.0f, 1.0f, wetLevel);
    if (std::abs(currentWetLevel - newWetLevel) > 0.001f)
    {
        currentWetLevel = newWetLevel;
        updateMixLevels();
    }
}

//==============================================================================
void FreOscReverb::updateImpulseResponse()
{
    // Calculate impulse parameters based on room size (matching JavaScript implementation)
    float duration = 0.5f + currentRoomSize * 3.0f; // 0.5 to 3.5 seconds
    float decay = 2.0f + currentRoomSize * 3.0f;     // Exponential decay factor

    // Generate new impulse response
    auto newImpulse = generateImpulseResponse(duration, decay);

    // Load the impulse into the convolution engine
    convolution.loadImpulseResponse(std::move(newImpulse), sampleRate,
                                   juce::dsp::Convolution::Stereo::yes,
                                   juce::dsp::Convolution::Trim::yes,
                                   juce::dsp::Convolution::Normalise::yes);
}

juce::AudioBuffer<float> FreOscReverb::generateImpulseResponse(float duration, float decay, bool reverse)
{
    // Calculate buffer size
    int length = static_cast<int>(sampleRate * duration);

    // Create stereo impulse buffer
    juce::AudioBuffer<float> impulse(2, length);

    // Generate impulse for both channels
    for (int channel = 0; channel < 2; ++channel)
    {
        auto* channelData = impulse.getWritePointer(channel);

        // Random number generator for noise
        juce::Random random(juce::Time::getCurrentTime().getMilliseconds() + channel * 1000);

        for (int i = 0; i < length; ++i)
        {
            // Calculate sample index (reverse if requested)
            int n = reverse ? length - i : i;

            // Generate white noise sample (-1 to 1)
            float noiseSample = random.nextFloat() * 2.0f - 1.0f;

            // Apply exponential decay envelope
            float envelope = std::pow(1.0f - static_cast<float>(n) / static_cast<float>(length), decay);

            // Combine noise with envelope
            channelData[i] = noiseSample * envelope;
        }
    }

    // Add slight stereo decorrelation for more natural sound
    if (impulse.getNumChannels() >= 2)
    {
        // Apply small delay difference between channels (up to 1ms)
        int maxDelay = static_cast<int>(sampleRate * 0.001f);
        juce::Random delayRandom(42); // Fixed seed for consistent results
        int delayOffset = delayRandom.nextInt(maxDelay);

        // Shift right channel slightly
        auto* rightChannel = impulse.getWritePointer(1);
        for (int i = impulse.getNumSamples() - 1; i >= delayOffset; --i)
        {
            rightChannel[i] = rightChannel[i - delayOffset];
        }

        // Clear the beginning of right channel
        for (int i = 0; i < delayOffset; ++i)
        {
            rightChannel[i] = 0.0f;
        }
    }

    return impulse;
}

void FreOscReverb::updateMixLevels()
{
    // Set wet and dry levels for proper mixing
    float wet = currentWetLevel;
    float dry = 1.0f - wet;

    wetGain.setGainLinear(wet);
    dryGain.setGainLinear(dry);
}

void FreOscReverb::processAlgorithmicReverb(const juce::dsp::ProcessContextReplacing<float>& context)
{
    auto& inputBlock = context.getInputBlock();
    auto& outputBlock = context.getOutputBlock();

    // Much more conservative feedback scaling to prevent instability
    float roomScale = 0.2f + currentRoomSize * 0.3f; // 0.2 to 0.5 feedback scaling

    for (size_t channel = 0; channel < inputBlock.getNumChannels(); ++channel)
    {
        auto* input = inputBlock.getChannelPointer(channel);
        auto* output = outputBlock.getChannelPointer(channel);

        for (size_t sampleIndex = 0; sampleIndex < inputBlock.getNumSamples(); ++sampleIndex)
        {
            float inputSample = input[sampleIndex];

            // Clamp input to prevent clipping
            inputSample = juce::jlimit(-1.0f, 1.0f, inputSample);

            float reverbSum = 0.0f;

            // Use only 4 delay lines to reduce complexity and potential for instability
            for (int i = 0; i < 4; ++i)
            {
                // Get delayed sample
                float delayedSample = delayLines[i].popSample(static_cast<int>(channel));

                // Apply light damping to all delay lines
                delayedSample = delayedSample * 0.9f; // Simple high-frequency roll-off

                // Add to reverb sum with simple panning
                float panFactor = (channel == 0) ? 1.0f : 0.8f;
                reverbSum += delayedSample * panFactor * 0.5f; // Scale down individual contributions

                // Feed input + much reduced feedback back into delay line
                float feedbackAmount = feedback[i] * roomScale * 0.5f; // Extra reduction
                float feedbackSample = inputSample * 0.3f + delayedSample * feedbackAmount;

                // Clamp feedback to prevent buildup
                feedbackSample = juce::jlimit(-0.8f, 0.8f, feedbackSample);

                delayLines[i].pushSample(static_cast<int>(channel), feedbackSample);
            }

            // Mix dry and wet signals with conservative wet level
            float dry = inputSample * (1.0f - currentWetLevel * 0.8f); // Reduce wet impact
            float wet = reverbSum * currentWetLevel * 0.15f; // Much lower wet gain

            // Final output clamping to prevent clipping
            output[sampleIndex] = juce::jlimit(-1.0f, 1.0f, dry + wet);
        }
    }
}