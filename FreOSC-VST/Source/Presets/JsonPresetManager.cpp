#include "JsonPresetManager.h"

//==============================================================================
JsonPresetManager::JsonPresetManager()
{
}

JsonPresetManager::~JsonPresetManager()
{
}

//==============================================================================
void JsonPresetManager::initialize(const juce::File& folder)
{
    presetFolder = folder;
    
    // Create presets folder if it doesn't exist
    if (!presetFolder.exists())
        presetFolder.createDirectory();
    
    scanForPresets();
}

void JsonPresetManager::scanForPresets()
{
    presets.clear();
    currentPresetIndex = -1;
    
    if (!presetFolder.exists())
        return;
    
    // Scan for .json files
    auto presetFiles = presetFolder.findChildFiles(juce::File::findFiles, false, "*.json");
    
    for (const auto& file : presetFiles)
    {
        juce::String name = file.getFileNameWithoutExtension();
        bool isFactory = name.startsWith("Factory_");
        
        presets.emplace_back(name, file, isFactory);
        
        // Try to load metadata from file
        if (auto jsonText = file.loadFileAsString(); jsonText.isNotEmpty())
        {
            if (auto json = juce::JSON::parse(jsonText); json.isObject())
            {
                presets.back().jsonData = json;
                if (json.hasProperty("description"))
                    presets.back().description = json["description"].toString();
            }
        }
    }
    
    // Sort presets - factory first, then alphabetically
    std::sort(presets.begin(), presets.end(), [](const PresetInfo& a, const PresetInfo& b) {
        if (a.isFactory != b.isFactory)
            return a.isFactory > b.isFactory; // Factory presets first
        return a.name < b.name; // Then alphabetically
    });
}

//==============================================================================
bool JsonPresetManager::loadPreset(int presetIndex, juce::AudioProcessorValueTreeState& parameters)
{
    if (presetIndex < 0 || presetIndex >= static_cast<int>(presets.size()))
        return false;
    
    const auto& preset = presets[presetIndex];
    
    if (loadPresetFromFile(preset.file, parameters))
    {
        currentPresetIndex = presetIndex;
        return true;
    }
    
    return false;
}

bool JsonPresetManager::loadPreset(const juce::String& presetName, juce::AudioProcessorValueTreeState& parameters)
{
    for (size_t i = 0; i < presets.size(); ++i)
    {
        if (presets[i].name == presetName)
        {
            return loadPreset(static_cast<int>(i), parameters);
        }
    }
    return false;
}

bool JsonPresetManager::savePreset(const juce::String& presetName, juce::AudioProcessorValueTreeState& parameters)
{
    return saveCurrentAsPreset(presetName, parameters);
}

bool JsonPresetManager::saveCurrentAsPreset(const juce::String& name, juce::AudioProcessorValueTreeState& parameters)
{
    juce::File presetFile = presetFolder.getChildFile(name + ".json");
    return savePresetToFile(presetFile, name, "", parameters);
}

//==============================================================================
juce::StringArray JsonPresetManager::getPresetNames() const
{
    juce::StringArray names;
    for (const auto& preset : presets)
        names.add(preset.name);
    return names;
}

juce::String JsonPresetManager::getPresetName(int index) const
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
        return presets[index].name;
    return {};
}

juce::String JsonPresetManager::getPresetDescription(int index) const
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
        return presets[index].description;
    return {};
}

bool JsonPresetManager::isFactoryPreset(int index) const
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
        return presets[index].isFactory;
    return false;
}

//==============================================================================
bool JsonPresetManager::loadPresetFromFile(const juce::File& file, juce::AudioProcessorValueTreeState& parameters)
{
    if (!file.exists())
        return false;
    
    auto jsonText = file.loadFileAsString();
    if (jsonText.isEmpty())
        return false;
    
    auto json = juce::JSON::parse(jsonText);
    if (!json.isObject())
        return false;
    
    return applyPresetJson(json, parameters);
}

bool JsonPresetManager::savePresetToFile(const juce::File& file, const juce::String& name, const juce::String& description, juce::AudioProcessorValueTreeState& parameters)
{
    auto json = createPresetJson(name, description, parameters);
    
    auto jsonText = juce::JSON::toString(json, true);
    
    return file.replaceWithText(jsonText);
}

//==============================================================================
juce::var JsonPresetManager::createPresetJson(const juce::String& name, const juce::String& description, juce::AudioProcessorValueTreeState& parameters)
{
    auto json = juce::DynamicObject::Ptr(new juce::DynamicObject());
    
    // Metadata
    json->setProperty("name", name);
    json->setProperty("description", description);
    json->setProperty("version", "1.0");
    
    // Oscillators
    auto oscillators = juce::DynamicObject::Ptr(new juce::DynamicObject());
    
    for (int i = 1; i <= 3; ++i)
    {
        auto osc = juce::DynamicObject::Ptr(new juce::DynamicObject());
        juce::String prefix = "osc" + juce::String(i) + "_";
        
        osc->setProperty("waveform", denormalizeOscillatorWaveform(parameters.getRawParameterValue(prefix + "waveform")->load()));
        osc->setProperty("octave", denormalizeOscillatorOctave(parameters.getRawParameterValue(prefix + "octave")->load()));
        osc->setProperty("level", parameters.getRawParameterValue(prefix + "level")->load());
        osc->setProperty("detune", denormalizeDetune(parameters.getRawParameterValue(prefix + "detune")->load()));
        osc->setProperty("pan", denormalizePan(parameters.getRawParameterValue(prefix + "pan")->load()));
        
        oscillators->setProperty("osc" + juce::String(i), osc.get());
    }
    json->setProperty("oscillators", oscillators.get());
    
    // Noise
    auto noise = juce::DynamicObject::Ptr(new juce::DynamicObject());
    noise->setProperty("type", denormalizeNoiseType(parameters.getRawParameterValue("noise_type")->load()));
    noise->setProperty("level", parameters.getRawParameterValue("noise_level")->load());
    noise->setProperty("pan", denormalizePan(parameters.getRawParameterValue("noise_pan")->load()));
    json->setProperty("noise", noise.get());
    
    // Envelope
    auto envelope = juce::DynamicObject::Ptr(new juce::DynamicObject());
    envelope->setProperty("attack", denormalizeTime(parameters.getRawParameterValue("envelope_attack")->load()));
    envelope->setProperty("decay", denormalizeTime(parameters.getRawParameterValue("envelope_decay")->load()));
    envelope->setProperty("sustain", parameters.getRawParameterValue("envelope_sustain")->load());
    envelope->setProperty("release", denormalizeTime(parameters.getRawParameterValue("envelope_release")->load()));
    json->setProperty("envelope", envelope.get());
    
    // Filter
    auto filter = juce::DynamicObject::Ptr(new juce::DynamicObject());
    filter->setProperty("type", denormalizeFilterType(parameters.getRawParameterValue("filter_type")->load()));
    filter->setProperty("cutoff", denormalizeFilterCutoff(parameters.getRawParameterValue("filter_cutoff")->load()));
    filter->setProperty("resonance", denormalizeFilterResonance(parameters.getRawParameterValue("filter_resonance")->load()));
    filter->setProperty("gain", denormalizeFilterGain(parameters.getRawParameterValue("filter_gain")->load()));
    filter->setProperty("vowel", denormalizeFormantVowel(parameters.getRawParameterValue("formant_vowel")->load()));
    json->setProperty("filter", filter.get());
    
    // LFO
    auto lfo = juce::DynamicObject::Ptr(new juce::DynamicObject());
    lfo->setProperty("waveform", denormalizeLfoWaveform(parameters.getRawParameterValue("lfo_waveform")->load()));
    lfo->setProperty("rate", denormalizeLfoRate(parameters.getRawParameterValue("lfo_rate")->load()));
    lfo->setProperty("target", denormalizeLfoTarget(parameters.getRawParameterValue("lfo_target")->load()));
    lfo->setProperty("amount", parameters.getRawParameterValue("lfo_amount")->load());
    json->setProperty("lfo", lfo.get());
    
    // Master
    json->setProperty("master_volume", parameters.getRawParameterValue("master_volume")->load());
    
    return juce::var(json.get());
}

bool JsonPresetManager::applyPresetJson(const juce::var& presetData, juce::AudioProcessorValueTreeState& parameters)
{
    if (!presetData.isObject())
        return false;
    
    auto json = presetData.getDynamicObject();
    if (json == nullptr)
        return false;
    
    // Apply oscillators
    if (json->hasProperty("oscillators"))
    {
        auto oscillators = json->getProperty("oscillators").getDynamicObject();
        if (oscillators != nullptr)
        {
            for (int i = 1; i <= 3; ++i)
            {
                juce::String oscKey = "osc" + juce::String(i);
                if (oscillators->hasProperty(oscKey))
                {
                    auto osc = oscillators->getProperty(oscKey).getDynamicObject();
                    if (osc != nullptr)
                    {
                        juce::String prefix = "osc" + juce::String(i) + "_";
                        
                        if (osc->hasProperty("waveform"))
                            parameters.getParameter(prefix + "waveform")->setValueNotifyingHost(
                                normalizeOscillatorWaveform(osc->getProperty("waveform")));
                        
                        if (osc->hasProperty("octave"))
                            parameters.getParameter(prefix + "octave")->setValueNotifyingHost(
                                normalizeOscillatorOctave(static_cast<int>(osc->getProperty("octave"))));
                        
                        if (osc->hasProperty("level"))
                            parameters.getParameter(prefix + "level")->setValueNotifyingHost(
                                static_cast<float>(osc->getProperty("level")));
                        
                        if (osc->hasProperty("detune"))
                            parameters.getParameter(prefix + "detune")->setValueNotifyingHost(
                                normalizeDetune(static_cast<float>(osc->getProperty("detune"))));
                        
                        if (osc->hasProperty("pan"))
                            parameters.getParameter(prefix + "pan")->setValueNotifyingHost(
                                normalizePan(static_cast<float>(osc->getProperty("pan"))));
                    }
                }
            }
        }
    }
    
    // Apply noise
    if (json->hasProperty("noise"))
    {
        auto noise = json->getProperty("noise").getDynamicObject();
        if (noise != nullptr)
        {
            if (noise->hasProperty("type"))
                parameters.getParameter("noise_type")->setValueNotifyingHost(
                    normalizeNoiseType(noise->getProperty("type")));
            
            if (noise->hasProperty("level"))
                parameters.getParameter("noise_level")->setValueNotifyingHost(
                    static_cast<float>(noise->getProperty("level")));
            
            if (noise->hasProperty("pan"))
                parameters.getParameter("noise_pan")->setValueNotifyingHost(
                    normalizePan(static_cast<float>(noise->getProperty("pan"))));
        }
    }
    
    // Apply envelope
    if (json->hasProperty("envelope"))
    {
        auto envelope = json->getProperty("envelope").getDynamicObject();
        if (envelope != nullptr)
        {
            if (envelope->hasProperty("attack"))
                parameters.getParameter("envelope_attack")->setValueNotifyingHost(
                    normalizeTime(static_cast<float>(envelope->getProperty("attack"))));
            
            if (envelope->hasProperty("decay"))
                parameters.getParameter("envelope_decay")->setValueNotifyingHost(
                    normalizeTime(static_cast<float>(envelope->getProperty("decay"))));
            
            if (envelope->hasProperty("sustain"))
                parameters.getParameter("envelope_sustain")->setValueNotifyingHost(
                    static_cast<float>(envelope->getProperty("sustain")));
            
            if (envelope->hasProperty("release"))
                parameters.getParameter("envelope_release")->setValueNotifyingHost(
                    normalizeTime(static_cast<float>(envelope->getProperty("release"))));
        }
    }
    
    // Apply filter
    if (json->hasProperty("filter"))
    {
        auto filter = json->getProperty("filter").getDynamicObject();
        if (filter != nullptr)
        {
            if (filter->hasProperty("type"))
                parameters.getParameter("filter_type")->setValueNotifyingHost(
                    normalizeFilterType(filter->getProperty("type")));
            
            if (filter->hasProperty("cutoff"))
                parameters.getParameter("filter_cutoff")->setValueNotifyingHost(
                    normalizeFilterCutoff(static_cast<float>(filter->getProperty("cutoff"))));
            
            if (filter->hasProperty("resonance"))
                parameters.getParameter("filter_resonance")->setValueNotifyingHost(
                    normalizeFilterResonance(static_cast<float>(filter->getProperty("resonance"))));
            
            if (filter->hasProperty("gain"))
                parameters.getParameter("filter_gain")->setValueNotifyingHost(
                    normalizeFilterGain(static_cast<float>(filter->getProperty("gain"))));
            
            if (filter->hasProperty("vowel"))
                parameters.getParameter("formant_vowel")->setValueNotifyingHost(
                    normalizeFormantVowel(filter->getProperty("vowel")));
        }
    }
    
    // Apply LFO
    if (json->hasProperty("lfo"))
    {
        auto lfo = json->getProperty("lfo").getDynamicObject();
        if (lfo != nullptr)
        {
            if (lfo->hasProperty("waveform"))
                parameters.getParameter("lfo_waveform")->setValueNotifyingHost(
                    normalizeLfoWaveform(lfo->getProperty("waveform")));
            
            if (lfo->hasProperty("rate"))
                parameters.getParameter("lfo_rate")->setValueNotifyingHost(
                    normalizeLfoRate(static_cast<float>(lfo->getProperty("rate"))));
            
            if (lfo->hasProperty("target"))
                parameters.getParameter("lfo_target")->setValueNotifyingHost(
                    normalizeLfoTarget(lfo->getProperty("target")));
            
            if (lfo->hasProperty("amount"))
                parameters.getParameter("lfo_amount")->setValueNotifyingHost(
                    static_cast<float>(lfo->getProperty("amount")));
        }
    }
    
    // Apply master volume
    if (json->hasProperty("master_volume"))
        parameters.getParameter("master_volume")->setValueNotifyingHost(
            static_cast<float>(json->getProperty("master_volume")));
    
    return true;
}

//==============================================================================
// Normalization helpers (JSON value -> 0-1 parameter value)

float JsonPresetManager::normalizeOscillatorWaveform(const juce::String& waveform)
{
    if (waveform == "sine") return 0.0f;
    if (waveform == "square") return 0.25f;
    if (waveform == "sawtooth") return 0.5f;
    if (waveform == "triangle") return 0.75f;
    return 0.0f; // Default to sine
}

float JsonPresetManager::normalizeOscillatorOctave(int octave)
{
    // Octave range: -2 to +2, normalized to 0-1
    return (octave + 2) / 4.0f;
}

float JsonPresetManager::normalizeDetune(float cents)
{
    // Detune range: -50 to +50 cents, normalized to 0-1
    return (cents + 50.0f) / 100.0f;
}

float JsonPresetManager::normalizePan(float pan)
{
    // Pan range: -1.0 to +1.0, normalized to 0-1
    return (pan + 1.0f) / 2.0f;
}

float JsonPresetManager::normalizeNoiseType(const juce::String& noiseType)
{
    if (noiseType == "white") return 0.0f;
    if (noiseType == "pink") return 0.1f;
    if (noiseType == "brown") return 0.2f;
    if (noiseType == "blue") return 0.3f;
    if (noiseType == "violet") return 0.4f;
    if (noiseType == "grey") return 0.5f;
    if (noiseType == "crackle") return 0.6f;
    if (noiseType == "digital") return 0.7f;
    if (noiseType == "wind") return 0.8f;
    if (noiseType == "ocean") return 0.9f;
    return 0.0f; // Default to white
}

float JsonPresetManager::normalizeFilterType(const juce::String& filterType)
{
    if (filterType == "lowpass") return 0.0f;
    if (filterType == "highpass") return 0.125f;
    if (filterType == "bandpass") return 0.25f;
    if (filterType == "notch") return 0.375f;
    if (filterType == "peaking") return 0.5f;
    if (filterType == "lowshelf") return 0.625f;
    if (filterType == "highshelf") return 0.75f;
    if (filterType == "allpass") return 0.875f;
    if (filterType == "formant") return 1.0f;
    return 0.0f; // Default to lowpass
}

float JsonPresetManager::normalizeFilterCutoff(float frequency)
{
    // Logarithmic scaling: 20Hz to 20kHz -> 0-1
    frequency = juce::jlimit(20.0f, 20000.0f, frequency);
    return std::log(frequency / 20.0f) / std::log(1000.0f); // 20 * 1000^x = frequency
}

float JsonPresetManager::normalizeFilterResonance(float q)
{
    // Q range: 0.1 to 30.0, normalized to 0-1
    return (q - 0.1f) / 29.9f;
}

float JsonPresetManager::normalizeFilterGain(float gainDb)
{
    // Gain range: -24dB to +24dB, normalized to 0-1
    return (gainDb + 24.0f) / 48.0f;
}

float JsonPresetManager::normalizeFormantVowel(const juce::String& vowel)
{
    if (vowel == "A") return 0.0f;
    if (vowel == "E") return 0.14f;
    if (vowel == "I") return 0.29f;
    if (vowel == "O") return 0.43f;
    if (vowel == "U") return 0.57f;
    if (vowel == "AE") return 0.71f;
    if (vowel == "AW") return 0.86f;
    if (vowel == "ER") return 1.0f;
    return 0.0f; // Default to A
}

float JsonPresetManager::normalizeLfoWaveform(const juce::String& waveform)
{
    if (waveform == "sine") return 0.0f;
    if (waveform == "triangle") return 0.25f;
    if (waveform == "sawtooth") return 0.5f;
    if (waveform == "square") return 0.75f;
    if (waveform == "random") return 1.0f;
    return 0.0f; // Default to sine
}

float JsonPresetManager::normalizeLfoRate(float hz)
{
    // LFO rate range: 0.1 to 20 Hz, normalized to 0-1
    hz = juce::jlimit(0.1f, 20.0f, hz);
    return (hz - 0.1f) / 19.9f;
}

float JsonPresetManager::normalizeLfoTarget(const juce::String& target)
{
    if (target == "none") return 0.0f;
    if (target == "pitch") return 0.25f;
    if (target == "filter") return 0.5f;
    if (target == "volume") return 0.75f;
    if (target == "pan") return 1.0f;
    return 0.0f; // Default to none
}

float JsonPresetManager::normalizeTime(float seconds)
{
    // Time range: 0.01 to 5.0 seconds, normalized to 0-1
    seconds = juce::jlimit(0.01f, 5.0f, seconds);
    return seconds / 5.0f;
}

//==============================================================================
// Denormalization helpers (0-1 parameter value -> JSON value)

juce::String JsonPresetManager::denormalizeOscillatorWaveform(float normalized)
{
    if (normalized < 0.125f) return "sine";
    if (normalized < 0.375f) return "square";
    if (normalized < 0.625f) return "sawtooth";
    return "triangle";
}

int JsonPresetManager::denormalizeOscillatorOctave(float normalized)
{
    return static_cast<int>(normalized * 4.0f) - 2;
}

float JsonPresetManager::denormalizeDetune(float normalized)
{
    return (normalized * 100.0f) - 50.0f;
}

float JsonPresetManager::denormalizePan(float normalized)
{
    return (normalized * 2.0f) - 1.0f;
}

juce::String JsonPresetManager::denormalizeNoiseType(float normalized)
{
    int index = static_cast<int>(normalized * 10.0f);
    switch (index)
    {
        case 0: return "white";
        case 1: return "pink";
        case 2: return "brown";
        case 3: return "blue";
        case 4: return "violet";
        case 5: return "grey";
        case 6: return "crackle";
        case 7: return "digital";
        case 8: return "wind";
        case 9: return "ocean";
        default: return "white";
    }
}

juce::String JsonPresetManager::denormalizeFilterType(float normalized)
{
    int index = static_cast<int>(normalized * 8.0f);
    switch (index)
    {
        case 0: return "lowpass";
        case 1: return "highpass";
        case 2: return "bandpass";
        case 3: return "notch";
        case 4: return "peaking";
        case 5: return "lowshelf";
        case 6: return "highshelf";
        case 7: return "allpass";
        case 8: return "formant";
        default: return "lowpass";
    }
}

float JsonPresetManager::denormalizeFilterCutoff(float normalized)
{
    // Logarithmic scaling: 0-1 -> 20Hz to 20kHz
    return 20.0f * std::pow(1000.0f, normalized);
}

float JsonPresetManager::denormalizeFilterResonance(float normalized)
{
    return 0.1f + (normalized * 29.9f);
}

float JsonPresetManager::denormalizeFilterGain(float normalized)
{
    return (normalized * 48.0f) - 24.0f;
}

juce::String JsonPresetManager::denormalizeFormantVowel(float normalized)
{
    int index = static_cast<int>(normalized * 7.0f);
    switch (index)
    {
        case 0: return "A";
        case 1: return "E";
        case 2: return "I";
        case 3: return "O";
        case 4: return "U";
        case 5: return "AE";
        case 6: return "AW";
        case 7: return "ER";
        default: return "A";
    }
}

juce::String JsonPresetManager::denormalizeLfoWaveform(float normalized)
{
    int index = static_cast<int>(normalized * 4.0f);
    switch (index)
    {
        case 0: return "sine";
        case 1: return "triangle";
        case 2: return "sawtooth";
        case 3: return "square";
        case 4: return "random";
        default: return "sine";
    }
}

float JsonPresetManager::denormalizeLfoRate(float normalized)
{
    return 0.1f + (normalized * 19.9f);
}

juce::String JsonPresetManager::denormalizeLfoTarget(float normalized)
{
    int index = static_cast<int>(normalized * 4.0f);
    switch (index)
    {
        case 0: return "none";
        case 1: return "pitch";
        case 2: return "filter";
        case 3: return "volume";
        case 4: return "pan";
        default: return "none";
    }
}

float JsonPresetManager::denormalizeTime(float normalized)
{
    return normalized * 5.0f;
}