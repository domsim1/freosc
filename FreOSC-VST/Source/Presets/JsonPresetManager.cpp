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

    // Collect preset files from multiple locations
    juce::Array<juce::File> allPresetFiles;
    
    // 1. Original preset folder (factory presets)
    if (presetFolder.exists())
    {
        auto factoryFiles = presetFolder.findChildFiles(juce::File::findFiles, false, "*.json");
        allPresetFiles.addArray(factoryFiles);
    }
    
    // 2. User's Documents/FreOSC/Presets folder
    auto documentsFolder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    auto userPresetFolder = documentsFolder.getChildFile("FreOSC").getChildFile("Presets");
    if (userPresetFolder.exists())
    {
        auto userFiles = userPresetFolder.findChildFiles(juce::File::findFiles, false, "*.json");
        allPresetFiles.addArray(userFiles);
    }
    
    // 3. Application data folder
    auto appDataFolder = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto appPresetFolder = appDataFolder.getChildFile("FreOSC").getChildFile("Presets");
    if (appPresetFolder.exists())
    {
        auto appFiles = appPresetFolder.findChildFiles(juce::File::findFiles, false, "*.json");
        allPresetFiles.addArray(appFiles);
    }

    for (const auto& file : allPresetFiles)
    {
        juce::String fileName = file.getFileNameWithoutExtension();
        bool isFactory = fileName.startsWith("Factory_");
        
        // For display name, remove Factory_ or User_ prefixes
        juce::String displayName = fileName;
        if (isFactory && displayName.startsWith("Factory_"))
        {
            displayName = displayName.substring(8); // Remove "Factory_" prefix
        }
        else if (displayName.startsWith("User_"))
        {
            displayName = displayName.substring(5); // Remove "User_" prefix
        }

        presets.emplace_back(displayName, file, isFactory);

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

    const auto& preset = presets[static_cast<size_t>(presetIndex)];

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
// New preset management methods

bool JsonPresetManager::updatePreset(int presetIndex, juce::AudioProcessorValueTreeState& parameters)
{
    if (presetIndex < 0 || presetIndex >= static_cast<int>(presets.size()))
        return false;
    
    const auto& preset = presets[static_cast<size_t>(presetIndex)];
    
    // Don't allow updating factory presets
    if (preset.isFactory)
        return false;
    
    // Update the preset file with current parameters
    if (savePresetToFile(preset.file, preset.name, preset.description, parameters))
    {
        // Refresh the preset data
        scanForPresets();
        return true;
    }
    
    return false;
}

bool JsonPresetManager::updatePreset(const juce::String& presetName, juce::AudioProcessorValueTreeState& parameters)
{
    for (size_t i = 0; i < presets.size(); ++i)
    {
        if (presets[i].name == presetName)
        {
            return updatePreset(static_cast<int>(i), parameters);
        }
    }
    return false;
}

bool JsonPresetManager::deletePreset(int presetIndex)
{
    if (presetIndex < 0 || presetIndex >= static_cast<int>(presets.size()))
        return false;
    
    const auto& preset = presets[static_cast<size_t>(presetIndex)];
    
    // Don't allow deleting factory presets
    if (preset.isFactory)
        return false;
    
    // Delete the file
    bool success = preset.file.deleteFile();
    
    if (success)
    {
        // Clear current preset if we just deleted it
        if (currentPresetIndex == presetIndex)
            currentPresetIndex = -1;
        else if (currentPresetIndex > presetIndex)
            currentPresetIndex--; // Adjust index after deletion
        
        // Refresh the preset list
        scanForPresets();
    }
    
    return success;
}

bool JsonPresetManager::deletePreset(const juce::String& presetName)
{
    for (size_t i = 0; i < presets.size(); ++i)
    {
        if (presets[i].name == presetName)
        {
            return deletePreset(static_cast<int>(i));
        }
    }
    return false;
}

bool JsonPresetManager::saveUserPreset(const juce::String& name, const juce::String& description, juce::AudioProcessorValueTreeState& parameters)
{
    // Create user preset filename (prefix with "User_" to distinguish from factory presets)
    juce::String filename = "User_" + name;
    
    // Try multiple locations for saving user presets
    juce::Array<juce::File> possibleLocations;
    
    // 1. Original preset folder (same as factory presets)
    possibleLocations.add(presetFolder.getChildFile(filename + ".json"));
    
    // 2. User's Documents/FreOSC/Presets folder
    auto documentsFolder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    auto userPresetFolder = documentsFolder.getChildFile("FreOSC").getChildFile("Presets");
    possibleLocations.add(userPresetFolder.getChildFile(filename + ".json"));
    
    // 3. Application data folder
    auto appDataFolder = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto appPresetFolder = appDataFolder.getChildFile("FreOSC").getChildFile("Presets");
    possibleLocations.add(appPresetFolder.getChildFile(filename + ".json"));
    
    // Try each location until one succeeds
    for (const auto& presetFile : possibleLocations)
    {
        DBG("Trying to save preset to: " + presetFile.getFullPathName());
        
        if (savePresetToFile(presetFile, name, description, parameters))
        {
            DBG("Successfully saved user preset to: " + presetFile.getFullPathName());
            // Refresh preset list to include new preset
            scanForPresets();
            return true;
        }
    }
    
    DBG("Failed to save preset to any location");
    return false;
}

bool JsonPresetManager::presetExists(const juce::String& presetName) const
{
    for (const auto& preset : presets)
    {
        if (preset.name == presetName)
            return true;
    }
    return false;
}

//==============================================================================
// Current preset state tracking methods

juce::String JsonPresetManager::getCurrentPresetName() const
{
    if (currentPresetIndex >= 0 && currentPresetIndex < static_cast<int>(presets.size()))
        return presets[static_cast<size_t>(currentPresetIndex)].name;
    return "Default";
}

void JsonPresetManager::setCurrentPreset(int index)
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
        currentPresetIndex = index;
    else
        currentPresetIndex = -1;
}

void JsonPresetManager::setCurrentPreset(const juce::String& name)
{
    for (size_t i = 0; i < presets.size(); ++i)
    {
        if (presets[i].name == name)
        {
            currentPresetIndex = static_cast<int>(i);
            return;
        }
    }
    currentPresetIndex = -1; // Not found, set to default
}

void JsonPresetManager::clearCurrentPreset()
{
    currentPresetIndex = -1; // This will make getCurrentPresetName() return "Default"
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
        return presets[static_cast<size_t>(index)].name;
    return {};
}

juce::String JsonPresetManager::getPresetDescription(int index) const
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
        return presets[static_cast<size_t>(index)].description;
    return {};
}

bool JsonPresetManager::isFactoryPreset(int index) const
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
        return presets[static_cast<size_t>(index)].isFactory;
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
    // Ensure the parent directory exists
    if (!file.getParentDirectory().exists())
    {
        if (!file.getParentDirectory().createDirectory())
        {
            DBG("Failed to create preset directory: " + file.getParentDirectory().getFullPathName());
            return false;
        }
    }
    
    // Use simple format for compatibility with factory presets and easier loading
    auto json = createSimplePresetJson(name, description, parameters);

    auto jsonText = juce::JSON::toString(json, true);
    
    DBG("Attempting to save preset to: " + file.getFullPathName());
    DBG("JSON content: " + jsonText);

    bool result = file.replaceWithText(jsonText);
    
    if (!result)
    {
        DBG("Failed to save preset file: " + file.getFullPathName());
    }
    else
    {
        DBG("Successfully saved preset: " + name);
    }

    return result;
}

//==============================================================================

juce::var JsonPresetManager::createSimplePresetJson(const juce::String& name, const juce::String& description, juce::AudioProcessorValueTreeState& parameters)
{
    auto json = juce::DynamicObject::Ptr(new juce::DynamicObject());
    
    // Metadata (same as factory presets)
    json->setProperty("name", name);
    json->setProperty("description", description);
    
    // Parameters object containing direct parameter mappings (like factory presets)
    auto parametersObj = juce::DynamicObject::Ptr(new juce::DynamicObject());
    
    // Get all parameter values using the same method as the complex format
    // Define all parameter IDs that should be saved (based on existing factory presets)
    juce::StringArray parameterIDs = {
        // Oscillators
        "osc1_waveform", "osc1_octave", "osc1_level", "osc1_detune", "osc1_pan",
        "osc2_waveform", "osc2_octave", "osc2_level", "osc2_detune", "osc2_pan", 
        "osc3_waveform", "osc3_octave", "osc3_level", "osc3_detune", "osc3_pan",
        
        // Noise
        "noise_type", "noise_level", "noise_pan",
        
        // Envelope
        "envelope_attack", "envelope_decay", "envelope_sustain", "envelope_release",
        
        // Filters
        "filter_routing", "filter_type", "filter_cutoff", "filter_resonance", "filter_gain",
        "filter2_type", "filter2_cutoff", "filter2_resonance", "filter2_gain",
        
        // LFO
        "lfo_waveform", "lfo_rate", "lfo_target", "lfo_amount",
        "lfo2_waveform", "lfo2_rate", "lfo2_target", "lfo2_amount",
        "lfo3_waveform", "lfo3_rate", "lfo3_target", "lfo3_amount",
        
        // Modulation Envelopes
        "mod_env1_attack", "mod_env1_decay", "mod_env1_sustain", "mod_env1_release",
        "mod_env1_amount", "mod_env1_target", "mod_env1_mode", "mod_env1_rate",
        "mod_env2_attack", "mod_env2_decay", "mod_env2_sustain", "mod_env2_release", 
        "mod_env2_amount", "mod_env2_target", "mod_env2_mode", "mod_env2_rate",
        
        // PM Synthesis
        "pm_index", "pm_ratio", "pm_carrier",
        
        
        
        // Effects
        "effects_routing",
        "plate_predelay", "plate_size", "plate_damping", "plate_diffusion", "plate_wet_level", "plate_width",
        "tape_time", "tape_feedback", "tape_tone", "tape_flutter", "tape_wet_level", "tape_width",
        "wavefolder_drive", "wavefolder_threshold", "wavefolder_symmetry", "wavefolder_mix", "wavefolder_output",
        
        // Master
        "master_volume"
    };
    
    // Save each parameter using normalized values (0-1) for consistency with loading
    // int paramCount = 0;
    for (const auto& paramID : parameterIDs)
    {
        if (auto* param = parameters.getParameter(paramID))
        {
            float normalizedValue = param->getValue(); // Gets normalized 0-1 value
            parametersObj->setProperty(paramID, normalizedValue);
            // paramCount++;
        }
        else
        {
            DBG("Parameter not found: " + paramID);
        }
    }
    
    // DBG("Saved " + juce::String(paramCount) + " parameters to preset: " + name);
    
    json->setProperty("parameters", parametersObj.get());
    
    return juce::var(json.get());
}

bool JsonPresetManager::applyPresetJson(const juce::var& presetData, juce::AudioProcessorValueTreeState& parameters)
{
    if (!presetData.isObject())
        return false;

    auto json = presetData.getDynamicObject();
    if (json == nullptr)
        return false;

    // Check if this is the simple format (has "parameters" object) used by factory presets
    if (json->hasProperty("parameters"))
    {
        return applySimplePresetFormat(json->getProperty("parameters"), parameters);
    }

    // Only support simple format - complex format no longer used
    return false;
}

//==============================================================================
// Simple preset format handler (for factory presets and simple user presets)

bool JsonPresetManager::applySimplePresetFormat(const juce::var& parametersData, juce::AudioProcessorValueTreeState& parameters)
{
    if (!parametersData.isObject())
        return false;
    
    auto paramObj = parametersData.getDynamicObject();
    if (paramObj == nullptr)
        return false;
    
    // Apply parameters directly by name - values are normalized (0-1)
    for (auto it = paramObj->getProperties().begin(); it != paramObj->getProperties().end(); ++it)
    {
        juce::String paramName = it->name.toString();
        float paramValue = static_cast<float>(it->value);
        
        // Set the normalized parameter value directly
        // Skip compressor and limiter parameters to ensure they always use default values
        if (paramName.startsWith("comp_") || paramName.startsWith("limiter_"))
        {
            DBG("Skipping compressor/limiter parameter: " + paramName + " to enforce default values.");
            continue;
        }

        if (auto* param = parameters.getParameter(paramName))
        {
            param->setValueNotifyingHost(paramValue);
        }
    }
    
    // All parameters are included in new presets - no defaults needed
    
    return true;
}

// No conversion helpers needed - using normalized values directly

