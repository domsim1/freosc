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

    // Filter system
    auto filterSystem = juce::DynamicObject::Ptr(new juce::DynamicObject());
    
    // Filter routing
    filterSystem->setProperty("routing", denormalizeFilterRouting(parameters.getRawParameterValue("filter_routing")->load()));
    
    // Filter 1
    auto filter1 = juce::DynamicObject::Ptr(new juce::DynamicObject());
    filter1->setProperty("type", denormalizeFilterType(parameters.getRawParameterValue("filter_type")->load()));
    filter1->setProperty("cutoff", denormalizeFilterCutoff(parameters.getRawParameterValue("filter_cutoff")->load()));
    filter1->setProperty("resonance", denormalizeFilterResonance(parameters.getRawParameterValue("filter_resonance")->load()));
    filter1->setProperty("gain", denormalizeFilterGain(parameters.getRawParameterValue("filter_gain")->load()));
    filterSystem->setProperty("filter1", filter1.get());
    
    // Filter 2
    auto filter2 = juce::DynamicObject::Ptr(new juce::DynamicObject());
    filter2->setProperty("type", denormalizeFilterType(parameters.getRawParameterValue("filter2_type")->load()));
    filter2->setProperty("cutoff", denormalizeFilterCutoff(parameters.getRawParameterValue("filter2_cutoff")->load()));
    filter2->setProperty("resonance", denormalizeFilterResonance(parameters.getRawParameterValue("filter2_resonance")->load()));
    filter2->setProperty("gain", denormalizeFilterGain(parameters.getRawParameterValue("filter2_gain")->load()));
    filterSystem->setProperty("filter2", filter2.get());
    
    json->setProperty("filterSystem", filterSystem.get());

    // LFO
    auto lfo = juce::DynamicObject::Ptr(new juce::DynamicObject());
    lfo->setProperty("waveform", denormalizeLfoWaveform(parameters.getRawParameterValue("lfo_waveform")->load()));
    lfo->setProperty("rate", denormalizeLfoRate(parameters.getRawParameterValue("lfo_rate")->load()));
    lfo->setProperty("target", denormalizeLfoTarget(parameters.getRawParameterValue("lfo_target")->load()));
    lfo->setProperty("amount", parameters.getRawParameterValue("lfo_amount")->load());
    json->setProperty("lfo", lfo.get());

    // Modulation Envelopes
    auto modulation = juce::DynamicObject::Ptr(new juce::DynamicObject());
    
    // Modulation Envelope 1
    auto modEnv1 = juce::DynamicObject::Ptr(new juce::DynamicObject());
    modEnv1->setProperty("attack", denormalizeTime(parameters.getRawParameterValue("mod_env1_attack")->load()));
    modEnv1->setProperty("decay", denormalizeTime(parameters.getRawParameterValue("mod_env1_decay")->load()));
    modEnv1->setProperty("sustain", parameters.getRawParameterValue("mod_env1_sustain")->load());
    modEnv1->setProperty("release", denormalizeTime(parameters.getRawParameterValue("mod_env1_release")->load()));
    modEnv1->setProperty("amount", parameters.getRawParameterValue("mod_env1_amount")->load());
    modEnv1->setProperty("target", denormalizeModEnvTarget(parameters.getRawParameterValue("mod_env1_target")->load()));
    modulation->setProperty("envelope1", modEnv1.get());
    
    // Modulation Envelope 2
    auto modEnv2 = juce::DynamicObject::Ptr(new juce::DynamicObject());
    modEnv2->setProperty("attack", denormalizeTime(parameters.getRawParameterValue("mod_env2_attack")->load()));
    modEnv2->setProperty("decay", denormalizeTime(parameters.getRawParameterValue("mod_env2_decay")->load()));
    modEnv2->setProperty("sustain", parameters.getRawParameterValue("mod_env2_sustain")->load());
    modEnv2->setProperty("release", denormalizeTime(parameters.getRawParameterValue("mod_env2_release")->load()));
    modEnv2->setProperty("amount", parameters.getRawParameterValue("mod_env2_amount")->load());
    modEnv2->setProperty("target", denormalizeModEnvTarget(parameters.getRawParameterValue("mod_env2_target")->load()));
    modulation->setProperty("envelope2", modEnv2.get());
    
    json->setProperty("modulation", modulation.get());

    // Effects
    auto effects = juce::DynamicObject::Ptr(new juce::DynamicObject());
    
    // Effects routing
    effects->setProperty("routing", denormalizeEffectsRouting(parameters.getRawParameterValue("effects_routing")->load()));
    
    // Plate Reverb
    auto plateReverb = juce::DynamicObject::Ptr(new juce::DynamicObject());
    plateReverb->setProperty("predelay", parameters.getRawParameterValue("plate_predelay")->load());
    plateReverb->setProperty("size", parameters.getRawParameterValue("plate_size")->load());
    plateReverb->setProperty("damping", parameters.getRawParameterValue("plate_damping")->load());
    plateReverb->setProperty("diffusion", parameters.getRawParameterValue("plate_diffusion")->load());
    plateReverb->setProperty("wetLevel", parameters.getRawParameterValue("plate_wet_level")->load());
    plateReverb->setProperty("width", parameters.getRawParameterValue("plate_width")->load());
    effects->setProperty("plateReverb", plateReverb.get());
    
    // Tape Delay
    auto tapeDelay = juce::DynamicObject::Ptr(new juce::DynamicObject());
    tapeDelay->setProperty("time", parameters.getRawParameterValue("tape_time")->load());
    tapeDelay->setProperty("feedback", parameters.getRawParameterValue("tape_feedback")->load());
    tapeDelay->setProperty("tone", parameters.getRawParameterValue("tape_tone")->load());
    tapeDelay->setProperty("flutter", parameters.getRawParameterValue("tape_flutter")->load());
    tapeDelay->setProperty("wetLevel", parameters.getRawParameterValue("tape_wet_level")->load());
    tapeDelay->setProperty("width", parameters.getRawParameterValue("tape_width")->load());
    effects->setProperty("tapeDelay", tapeDelay.get());
    
    json->setProperty("effects", effects.get());

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

    // Apply filter system (new format) or legacy filter (old format)
    if (json->hasProperty("filterSystem"))
    {
        auto filterSystem = json->getProperty("filterSystem").getDynamicObject();
        if (filterSystem != nullptr)
        {
            // Apply filter routing
            if (filterSystem->hasProperty("routing"))
                parameters.getParameter("filter_routing")->setValueNotifyingHost(
                    normalizeFilterRouting(filterSystem->getProperty("routing")));
            
            // Apply Filter 1
            if (filterSystem->hasProperty("filter1"))
            {
                auto filter1 = filterSystem->getProperty("filter1").getDynamicObject();
                if (filter1 != nullptr)
                {
                    if (filter1->hasProperty("type"))
                        parameters.getParameter("filter_type")->setValueNotifyingHost(
                            normalizeFilterType(filter1->getProperty("type")));

                    if (filter1->hasProperty("cutoff"))
                        parameters.getParameter("filter_cutoff")->setValueNotifyingHost(
                            normalizeFilterCutoff(static_cast<float>(filter1->getProperty("cutoff"))));

                    if (filter1->hasProperty("resonance"))
                        parameters.getParameter("filter_resonance")->setValueNotifyingHost(
                            normalizeFilterResonance(static_cast<float>(filter1->getProperty("resonance"))));

                    if (filter1->hasProperty("gain"))
                        parameters.getParameter("filter_gain")->setValueNotifyingHost(
                            normalizeFilterGain(static_cast<float>(filter1->getProperty("gain"))));
                }
            }
            
            // Apply Filter 2
            if (filterSystem->hasProperty("filter2"))
            {
                auto filter2 = filterSystem->getProperty("filter2").getDynamicObject();
                if (filter2 != nullptr)
                {
                    if (filter2->hasProperty("type"))
                        parameters.getParameter("filter2_type")->setValueNotifyingHost(
                            normalizeFilterType(filter2->getProperty("type")));

                    if (filter2->hasProperty("cutoff"))
                        parameters.getParameter("filter2_cutoff")->setValueNotifyingHost(
                            normalizeFilterCutoff(static_cast<float>(filter2->getProperty("cutoff"))));

                    if (filter2->hasProperty("resonance"))
                        parameters.getParameter("filter2_resonance")->setValueNotifyingHost(
                            normalizeFilterResonance(static_cast<float>(filter2->getProperty("resonance"))));

                    if (filter2->hasProperty("gain"))
                        parameters.getParameter("filter2_gain")->setValueNotifyingHost(
                            normalizeFilterGain(static_cast<float>(filter2->getProperty("gain"))));
                }
            }
        }
    }
    else if (json->hasProperty("filter"))
    {
        // Legacy filter format - load as Filter 1 only
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

    // Apply modulation envelopes
    if (json->hasProperty("modulation"))
    {
        auto modulation = json->getProperty("modulation").getDynamicObject();
        if (modulation != nullptr)
        {
            // Apply Modulation Envelope 1
            if (modulation->hasProperty("envelope1"))
            {
                auto modEnv1 = modulation->getProperty("envelope1").getDynamicObject();
                if (modEnv1 != nullptr)
                {
                    if (modEnv1->hasProperty("attack"))
                        parameters.getParameter("mod_env1_attack")->setValueNotifyingHost(
                            normalizeTime(static_cast<float>(modEnv1->getProperty("attack"))));

                    if (modEnv1->hasProperty("decay"))
                        parameters.getParameter("mod_env1_decay")->setValueNotifyingHost(
                            normalizeTime(static_cast<float>(modEnv1->getProperty("decay"))));

                    if (modEnv1->hasProperty("sustain"))
                        parameters.getParameter("mod_env1_sustain")->setValueNotifyingHost(
                            static_cast<float>(modEnv1->getProperty("sustain")));

                    if (modEnv1->hasProperty("release"))
                        parameters.getParameter("mod_env1_release")->setValueNotifyingHost(
                            normalizeTime(static_cast<float>(modEnv1->getProperty("release"))));

                    if (modEnv1->hasProperty("amount"))
                        parameters.getParameter("mod_env1_amount")->setValueNotifyingHost(
                            static_cast<float>(modEnv1->getProperty("amount")));

                    if (modEnv1->hasProperty("target"))
                        parameters.getParameter("mod_env1_target")->setValueNotifyingHost(
                            normalizeModEnvTarget(modEnv1->getProperty("target")));
                }
            }

            // Apply Modulation Envelope 2
            if (modulation->hasProperty("envelope2"))
            {
                auto modEnv2 = modulation->getProperty("envelope2").getDynamicObject();
                if (modEnv2 != nullptr)
                {
                    if (modEnv2->hasProperty("attack"))
                        parameters.getParameter("mod_env2_attack")->setValueNotifyingHost(
                            normalizeTime(static_cast<float>(modEnv2->getProperty("attack"))));

                    if (modEnv2->hasProperty("decay"))
                        parameters.getParameter("mod_env2_decay")->setValueNotifyingHost(
                            normalizeTime(static_cast<float>(modEnv2->getProperty("decay"))));

                    if (modEnv2->hasProperty("sustain"))
                        parameters.getParameter("mod_env2_sustain")->setValueNotifyingHost(
                            static_cast<float>(modEnv2->getProperty("sustain")));

                    if (modEnv2->hasProperty("release"))
                        parameters.getParameter("mod_env2_release")->setValueNotifyingHost(
                            normalizeTime(static_cast<float>(modEnv2->getProperty("release"))));

                    if (modEnv2->hasProperty("amount"))
                        parameters.getParameter("mod_env2_amount")->setValueNotifyingHost(
                            static_cast<float>(modEnv2->getProperty("amount")));

                    if (modEnv2->hasProperty("target"))
                        parameters.getParameter("mod_env2_target")->setValueNotifyingHost(
                            normalizeModEnvTarget(modEnv2->getProperty("target")));
                }
            }
        }
    }

    // Apply effects
    if (json->hasProperty("effects"))
    {
        auto effects = json->getProperty("effects").getDynamicObject();
        if (effects != nullptr)
        {
            // Apply effects routing
            if (effects->hasProperty("routing"))
                parameters.getParameter("effects_routing")->setValueNotifyingHost(
                    normalizeEffectsRouting(effects->getProperty("routing")));
            
            // Apply plate reverb
            if (effects->hasProperty("plateReverb"))
            {
                auto plateReverb = effects->getProperty("plateReverb").getDynamicObject();
                if (plateReverb != nullptr)
                {
                    if (plateReverb->hasProperty("predelay"))
                        parameters.getParameter("plate_predelay")->setValueNotifyingHost(
                            static_cast<float>(plateReverb->getProperty("predelay")));

                    if (plateReverb->hasProperty("size"))
                        parameters.getParameter("plate_size")->setValueNotifyingHost(
                            static_cast<float>(plateReverb->getProperty("size")));

                    if (plateReverb->hasProperty("damping"))
                        parameters.getParameter("plate_damping")->setValueNotifyingHost(
                            static_cast<float>(plateReverb->getProperty("damping")));

                    if (plateReverb->hasProperty("diffusion"))
                        parameters.getParameter("plate_diffusion")->setValueNotifyingHost(
                            static_cast<float>(plateReverb->getProperty("diffusion")));

                    if (plateReverb->hasProperty("wetLevel"))
                        parameters.getParameter("plate_wet_level")->setValueNotifyingHost(
                            static_cast<float>(plateReverb->getProperty("wetLevel")));

                    if (plateReverb->hasProperty("width"))
                        parameters.getParameter("plate_width")->setValueNotifyingHost(
                            static_cast<float>(plateReverb->getProperty("width")));
                }
            }

            // Apply tape delay (new format)
            if (effects->hasProperty("tapeDelay"))
            {
                auto tapeDelay = effects->getProperty("tapeDelay").getDynamicObject();
                if (tapeDelay != nullptr)
                {
                    if (tapeDelay->hasProperty("time"))
                        parameters.getParameter("tape_time")->setValueNotifyingHost(
                            static_cast<float>(tapeDelay->getProperty("time")));

                    if (tapeDelay->hasProperty("feedback"))
                        parameters.getParameter("tape_feedback")->setValueNotifyingHost(
                            static_cast<float>(tapeDelay->getProperty("feedback")));

                    if (tapeDelay->hasProperty("tone"))
                        parameters.getParameter("tape_tone")->setValueNotifyingHost(
                            static_cast<float>(tapeDelay->getProperty("tone")));

                    if (tapeDelay->hasProperty("flutter"))
                        parameters.getParameter("tape_flutter")->setValueNotifyingHost(
                            static_cast<float>(tapeDelay->getProperty("flutter")));

                    if (tapeDelay->hasProperty("wetLevel"))
                        parameters.getParameter("tape_wet_level")->setValueNotifyingHost(
                            static_cast<float>(tapeDelay->getProperty("wetLevel")));

                    if (tapeDelay->hasProperty("width"))
                        parameters.getParameter("tape_width")->setValueNotifyingHost(
                            static_cast<float>(tapeDelay->getProperty("width")));
                }
            }
            // Apply legacy delay format for backward compatibility
            else if (effects->hasProperty("delay"))
            {
                auto delay = effects->getProperty("delay").getDynamicObject();
                if (delay != nullptr)
                {
                    // Map old delay parameters to new tape delay parameters
                    if (delay->hasProperty("time"))
                        parameters.getParameter("tape_time")->setValueNotifyingHost(
                            static_cast<float>(delay->getProperty("time")));

                    if (delay->hasProperty("feedback"))
                        parameters.getParameter("tape_feedback")->setValueNotifyingHost(
                            static_cast<float>(delay->getProperty("feedback")));

                    if (delay->hasProperty("wetLevel"))
                        parameters.getParameter("tape_wet_level")->setValueNotifyingHost(
                            static_cast<float>(delay->getProperty("wetLevel")));

                    // Set default values for new tape delay parameters not in old format
                    parameters.getParameter("tape_tone")->setValueNotifyingHost(0.7f);     // Mid tone
                    parameters.getParameter("tape_flutter")->setValueNotifyingHost(0.1f);  // Light flutter
                    parameters.getParameter("tape_width")->setValueNotifyingHost(0.5f);    // Centered width
                }
            }
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
    if (filterType == "highpass") return 0.5f;
    if (filterType == "bandpass") return 1.0f;
    return 0.0f; // Default to lowpass
}

float JsonPresetManager::normalizeFilterRouting(const juce::String& routing)
{
    if (routing == "off") return 0.0f;
    if (routing == "parallel") return 1.0f;
    if (routing == "series") return 2.0f;
    return 0.0f; // Default to off
}

float JsonPresetManager::normalizeEffectsRouting(const juce::String& routing)
{
    if (routing == "series_reverb_delay") return 0.0f;
    if (routing == "series_delay_reverb") return 1.0f;
    if (routing == "parallel") return 2.0f;
    return 0.0f; // Default to series reverb→delay
}

float JsonPresetManager::normalizeFilterCutoff(float frequency)
{
    // Logarithmic scaling: 20Hz to 20kHz -> 0-1
    frequency = juce::jlimit(20.0f, 20000.0f, frequency);
    return std::log(frequency / 20.0f) / std::log(1000.0f); // 20 * 1000^x = frequency
}

float JsonPresetManager::normalizeFilterResonance(float q)
{
    // Q range: 0.1 to 5.0, normalized to 0-1
    return (q - 0.1f) / 4.9f;
}

float JsonPresetManager::normalizeFilterGain(float gainDb)
{
    // Gain range: -24dB to +24dB, normalized to 0-1
    return (gainDb + 24.0f) / 48.0f;
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
    if (target == "pitch") return 0.2f;
    if (target == "filter") return 0.4f;
    if (target == "filter2") return 0.6f;
    if (target == "volume") return 0.8f;
    if (target == "pan") return 1.0f;
    return 0.0f; // Default to none
}

float JsonPresetManager::normalizeModEnvTarget(const juce::String& target)
{
    if (target == "none") return 0.0f;
    if (target == "fm_amount") return 1.0f;
    if (target == "fm_ratio") return 2.0f;
    if (target == "filter_cutoff") return 3.0f;
    if (target == "filter2_cutoff") return 4.0f;
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
    int index = static_cast<int>(normalized * 2.0f);
    switch (index)
    {
        case 0: return "lowpass";
        case 1: return "highpass";
        case 2: return "bandpass";
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
    return 0.1f + (normalized * 4.9f);
}

float JsonPresetManager::denormalizeFilterGain(float normalized)
{
    return (normalized * 48.0f) - 24.0f;
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
    int index = static_cast<int>(normalized * 5.0f);
    switch (index)
    {
        case 0: return "none";
        case 1: return "pitch";
        case 2: return "filter";
        case 3: return "filter2";
        case 4: return "volume";
        case 5: return "pan";
        default: return "none";
    }
}

float JsonPresetManager::denormalizeTime(float normalized)
{
    return normalized * 5.0f;
}

juce::String JsonPresetManager::denormalizeFilterRouting(float normalized)
{
    int index = static_cast<int>(normalized);
    switch (index)
    {
        case 0: return "off";
        case 1: return "parallel";
        case 2: return "series";
        default: return "off";
    }
}

juce::String JsonPresetManager::denormalizeEffectsRouting(float normalized)
{
    int index = static_cast<int>(normalized);
    switch (index)
    {
        case 0: return "series_reverb_delay";
        case 1: return "series_delay_reverb";
        case 2: return "parallel";
        default: return "series_reverb_delay";
    }
}

juce::String JsonPresetManager::denormalizeModEnvTarget(float normalized)
{
    int index = static_cast<int>(normalized);
    switch (index)
    {
        case 0: return "none";
        case 1: return "fm_amount";
        case 2: return "fm_ratio";
        case 3: return "filter_cutoff";
        case 4: return "filter2_cutoff";
        default: return "none";
    }
}