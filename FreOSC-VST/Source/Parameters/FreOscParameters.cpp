#include "FreOscParameters.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout FreOscParameters::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Add all float parameters
    for (const auto& paramInfo : floatParameters)
    {
        params.push_back(createFloatParameter(paramInfo));
    }

    // Add all choice parameters
    for (const auto& [id, name, choices, defaultIndex] : choiceParameters)
    {
        params.push_back(createChoiceParameter(id, name, choices, defaultIndex));
    }

    // Add integer parameters (octave controls)
    params.push_back(createIntParameter("osc1_octave", "Osc1 Octave", -2, 2, 0));
    params.push_back(createIntParameter("osc2_octave", "Osc2 Octave", -2, 2, 0));
    params.push_back(createIntParameter("osc3_octave", "Osc3 Octave", -2, 2, -1));

    return { params.begin(), params.end() };
}

//==============================================================================
std::unique_ptr<juce::AudioParameterFloat> FreOscParameters::createFloatParameter(const ParameterInfo& info)
{
    return std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(info.id, 1),
        info.name,
        info.range,
        info.defaultValue,
        juce::AudioParameterFloatAttributes().withLabel(info.suffix)
    );
}

//==============================================================================
std::unique_ptr<juce::AudioParameterChoice> FreOscParameters::createChoiceParameter(
    const juce::String& id, const juce::String& name,
    const juce::StringArray& choices, int defaultIndex)
{
    return std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(id, 1),
        name,
        choices,
        defaultIndex
    );
}

//==============================================================================
std::unique_ptr<juce::AudioParameterInt> FreOscParameters::createIntParameter(
    const juce::String& id, const juce::String& name,
    int min, int max, int defaultValue)
{
    return std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID(id, 1),
        name,
        min,
        max,
        defaultValue
    );
}