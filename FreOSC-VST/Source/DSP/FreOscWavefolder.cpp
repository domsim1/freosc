#include "FreOscWavefolder.h"
#include <cmath>

//==============================================================================
FreOscWavefolder::FreOscWavefolder()
{
    // Initialize with safe default settings (effect disabled by default)
    drive = 1.0f;         // 1x gain (no drive)
    threshold = 0.7f;     // 70% threshold
    symmetry = 0.0f;      // Symmetric folding
    mix = 0.0f;           // 0% wet (effect bypassed by default)
    outputLevel = 0.5f;   // 50% output compensation
}

//==============================================================================
void FreOscWavefolder::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    
    // Prepare DC blocking filters (one per channel)
    dcBlockers.clear();
    dcBlockers.resize(static_cast<size_t>(spec.numChannels));
    
    for (auto& dcBlocker : dcBlockers)
    {
        dcBlocker.prepare(spec);
        
        // High-pass filter at ~5Hz to remove DC offset
        auto coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(
            sampleRate, 5.0f);
        dcBlocker.coefficients = coefficients;
        
        dcBlocker.reset();
    }
}

void FreOscWavefolder::reset()
{
    for (auto& dcBlocker : dcBlockers)
    {
        dcBlocker.reset();
    }
}

//==============================================================================
void FreOscWavefolder::setDrive(float newDrive)
{
    // Map 0-1 to 1x-10x gain for musical range
    drive = 1.0f + (newDrive * 9.0f);
}

void FreOscWavefolder::setThreshold(float newThreshold)
{
    // Map 0-1 to a more dramatic range for audible differences
    // 0.0 = very low threshold (lots of folding), 1.0 = high threshold (minimal folding)
    threshold = 0.05f + (newThreshold * 0.85f); // Range: 0.05 to 0.9
}

void FreOscWavefolder::setSymmetry(float newSymmetry)
{
    symmetry = juce::jlimit(0.0f, 1.0f, newSymmetry);
}

void FreOscWavefolder::setMix(float newMix)
{
    mix = juce::jlimit(0.0f, 1.0f, newMix);
}

void FreOscWavefolder::setOutputLevel(float newLevel)
{
    outputLevel = juce::jlimit(0.0f, 1.0f, newLevel);
}

//==============================================================================
float FreOscWavefolder::processSample(float sample, int channel)
{
    // Safety check for NaN/infinite inputs
    if (!std::isfinite(sample))
        return 0.0f;
    
    // Apply input drive with safety limiting
    float drivenSample = sample * juce::jlimit(0.1f, 10.0f, drive);
    
    // Apply wavefolder distortion
    float foldedSample = wavefold(drivenSample);
    
    // Apply output level compensation with safety check
    float outputSample = foldedSample * juce::jlimit(0.0f, 2.0f, outputLevel);
    
    // Apply DC blocking filter with bounds check
    if (channel >= 0 && channel < static_cast<int>(dcBlockers.size()))
    {
        outputSample = dcBlockers[static_cast<size_t>(channel)].processSample(outputSample);
    }
    
    // Final safety check for NaN/infinite and limiting
    if (!std::isfinite(outputSample))
        outputSample = 0.0f;
    
    outputSample = juce::jlimit(-1.0f, 1.0f, outputSample);
    
    return outputSample;
}

float FreOscWavefolder::wavefold(float sample)
{
    // Clamp input to prevent extreme values
    sample = juce::jlimit(-10.0f, 10.0f, sample);
    
    // Use threshold directly - it's already been mapped to a good range
    float safeThreshold = juce::jlimit(0.05f, 0.9f, threshold);
    
    // Calculate asymmetric thresholds based on symmetry parameter
    // symmetry = 0.0 -> symmetric folding (equal positive/negative thresholds)
    // symmetry = 1.0 -> maximum asymmetry (very different positive/negative behavior)
    
    float positiveThreshold, negativeThreshold;
    
    if (symmetry < 0.5f)
    {
        // Symmetry 0-50%: Adjust thresholds while keeping them balanced
        float asymmetryAmount = symmetry * 2.0f; // 0.0 to 1.0
        positiveThreshold = safeThreshold * (1.0f + asymmetryAmount * 0.5f);
        negativeThreshold = -safeThreshold * (1.0f - asymmetryAmount * 0.3f);
    }
    else
    {
        // Symmetry 50-100%: Create dramatic asymmetry
        float extremeAsymmetry = (symmetry - 0.5f) * 2.0f; // 0.0 to 1.0
        positiveThreshold = safeThreshold * (1.5f + extremeAsymmetry * 0.3f);
        negativeThreshold = -safeThreshold * (0.7f - extremeAsymmetry * 0.4f);
    }
    
    // Ensure thresholds stay in valid ranges
    positiveThreshold = juce::jlimit(0.1f, 0.95f, positiveThreshold);
    negativeThreshold = juce::jlimit(-0.95f, -0.1f, negativeThreshold);
    
    // Calculate asymmetric gains for additional character
    float positiveGain = 1.0f + symmetry * 0.3f; // Boost positive portion with asymmetry
    float negativeGain = 1.0f - symmetry * 0.2f; // Reduce negative portion with asymmetry
    
    float output = sample;
    
    // Improved wavefolder algorithm with more dramatic threshold response
    if (sample > positiveThreshold)
    {
        // Aggressive folding for positive portion
        float excess = sample - positiveThreshold;
        
        // Simple triangle wave folding - more audible than complex algorithm
        if (excess < (1.0f - positiveThreshold))
        {
            // First fold: reflect downward
            output = positiveThreshold - excess;
        }
        else
        {
            // Multiple folds: triangle wave pattern
            float foldCycles = excess / (1.0f - positiveThreshold);
            int wholeCycles = (int)foldCycles;
            float remainder = foldCycles - wholeCycles;
            
            if (wholeCycles % 2 == 0)
                output = positiveThreshold - remainder * (1.0f - positiveThreshold);
            else
                output = positiveThreshold + remainder * (1.0f - positiveThreshold);
        }
        output *= positiveGain; // Apply asymmetric gain
        
        // Add some saturation character for more dramatic effect
        output = std::tanh(output * 1.2f) / 1.2f;
    }
    else if (sample < negativeThreshold)
    {
        // Aggressive folding for negative portion  
        float excess = negativeThreshold - sample; // Always positive
        
        // Simple triangle wave folding
        if (excess < (negativeThreshold + 1.0f))
        {
            // First fold: reflect upward
            output = negativeThreshold + excess;
        }
        else
        {
            // Multiple folds: triangle wave pattern
            float foldCycles = excess / (negativeThreshold + 1.0f);
            int wholeCycles = (int)foldCycles;
            float remainder = foldCycles - wholeCycles;
            
            if (wholeCycles % 2 == 0)
                output = negativeThreshold + remainder * (negativeThreshold + 1.0f);
            else
                output = negativeThreshold - remainder * (negativeThreshold + 1.0f);
        }
        output *= negativeGain; // Apply asymmetric gain
        
        // Add some saturation character for more dramatic effect
        output = std::tanh(output * 1.2f) / 1.2f;
    }
    
    // Final safety clamp
    return juce::jlimit(-2.0f, 2.0f, output);
}