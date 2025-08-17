#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

//==============================================================================
/**
    FreOSC Plugin Editor

    Complete custom GUI recreating the FreOSC web interface using JUCE components.
    Features professional dark theme with organized parameter sections matching
    the original web design.

    Layout:
    - Header with title and preset selector
    - Three oscillator sections with waveform, octave, level, detune, pan
    - Noise section with type selector and level
    - Filter section with type, cutoff, resonance, formant vowel
    - Envelope ADSR section
    - Effects sections: Dynamics, Reverb, Delay
    - LFO and FM synthesis sections
    - Virtual keyboard at bottom
*/
class FreOscEditor : public juce::AudioProcessorEditor, private juce::Timer, private juce::Button::Listener
{
public:
    FreOscEditor (FreOscProcessor&);
    ~FreOscEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    //==============================================================================
    // Colors and styling (public for custom components)
    static const juce::Colour backgroundColour;
    static const juce::Colour panelColour;
    static const juce::Colour accentColour;
    static const juce::Colour textColour;
    static const juce::Colour knobColour;

private:
    //==============================================================================
    // Processor reference
    FreOscProcessor& audioProcessor;

    // Scaling for resizable interface
    float currentScale = 1.0f;

    // Minimum usable size before scrolling kicks in
    int minUsableWidth = 800;
    int minUsableHeight = 600;

    // Parameter attachments for automatic GUI updates
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;

    //==============================================================================
    // GUI Components

    // Header section
    juce::Label titleLabel;
    juce::ComboBox presetSelector;
    juce::TextButton loadPresetButton;

    // Scrollable content system
    juce::Viewport viewport;
    juce::Component contentComponent;

    // Tabbed interface
    juce::TabbedComponent tabbedComponent;
    
    // Custom look and feel for solid tabs
    std::unique_ptr<juce::LookAndFeel> customLookAndFeel;

    // Tab content components
    std::unique_ptr<juce::Component> oscillatorsTab;
    std::unique_ptr<juce::Component> filterEnvelopeTab;
    std::unique_ptr<juce::Component> modulationTab;
    std::unique_ptr<juce::Component> effectsTab;

    // Oscillator sections (3 identical sets)
    
    struct OscillatorSection
    {
        juce::GroupComponent group;
        juce::ComboBox waveformCombo;
        juce::ComboBox octaveCombo;
        juce::Slider levelSlider;
        juce::Slider detuneSlider;
        juce::Slider panSlider;
        juce::Label waveformLabel, octaveLabel, levelLabel, detuneLabel, panLabel;
        juce::Label levelValue, detuneValue, panValue;
    };

    OscillatorSection osc1Section, osc2Section, osc3Section;

    // Noise section
    juce::GroupComponent noiseGroup;
    juce::ComboBox noiseTypeCombo;
    juce::Slider noiseLevelSlider;
    juce::Slider noisePanSlider;
    juce::Label noiseTypeLabel, noiseLevelLabel, noisePanLabel;
    juce::Label noiseLevelValue, noisePanValue;

    // Master section
    juce::GroupComponent masterGroup;
    juce::Slider masterVolumeSlider;
    juce::Label masterVolumeLabel, masterVolumeValue;

    // Envelope section
    juce::GroupComponent envelopeGroup;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;
    juce::Label attackValue, decayValue, sustainValue, releaseValue;

    // Filter section
    juce::GroupComponent filterGroup;
    juce::ComboBox filterTypeCombo;
    juce::Slider cutoffSlider, resonanceSlider, filterGainSlider;
    juce::ComboBox formantVowelCombo;
    juce::Label filterTypeLabel, cutoffLabel, resonanceLabel, filterGainLabel, formantLabel;
    juce::Label cutoffValue, resonanceValue, filterGainValue;

    // FM synthesis section
    juce::GroupComponent fmGroup;
    juce::Slider fmAmountSlider, fmRatioSlider;
    juce::ComboBox fmTargetCombo;
    juce::Label fmAmountLabel, fmRatioLabel, fmTargetLabel;
    juce::Label fmAmountValue, fmRatioValue;

    // Dynamics section removed - now uses fixed internal settings

    // Reverb section
    juce::GroupComponent reverbGroup;
    juce::Slider roomSizeSlider, reverbWetSlider;
    juce::Label roomSizeLabel, reverbWetLabel;
    juce::Label roomSizeValue, reverbWetValue;

    // Delay section
    juce::GroupComponent delayGroup;
    juce::Slider delayTimeSlider, delayFeedbackSlider, delayWetSlider;
    juce::Label delayTimeLabel, delayFeedbackLabel, delayWetLabel;
    juce::Label delayTimeValue, delayFeedbackValue, delayWetValue;

    // LFO section
    juce::GroupComponent lfoGroup;
    juce::ComboBox lfoWaveformCombo, lfoTargetCombo;
    juce::Slider lfoRateSlider, lfoAmountSlider;
    juce::Label lfoWaveformLabel, lfoTargetLabel, lfoRateLabel, lfoAmountLabel;
    juce::Label lfoRateValue, lfoAmountValue;

    //==============================================================================
    // Helper methods
    void setupComponent(juce::Component& component);
    void setupSlider(juce::Slider& slider, juce::Label& label, juce::Label& valueLabel,
                    const juce::String& labelText, const juce::String& parameterID);
    void setupComboBox(juce::ComboBox& combo, juce::Label& label,
                      const juce::String& labelText, const juce::String& parameterID);
    void setupOscillatorSection(OscillatorSection& section, const juce::String& title,
                               const juce::String& paramPrefix);

    // Layout helpers
    juce::Rectangle<int> getOscillatorBounds(int index);
    juce::Rectangle<int> getSectionBounds(int row, int col, int width = 1, int height = 1);

    // Value formatting
    juce::String formatPanValue(float value);
    juce::String formatTimeValue(float value);
    juce::String formatFrequencyValue(float normalizedValue);
    juce::String formatResonanceValue(float normalizedValue);
    juce::String formatFilterGainValue(float normalizedValue);

    // Parameter update callbacks
    void updateValueLabels();
    void updateScaledFonts();

    // Timer callback for real-time updates
    void timerCallback() override;

    // Button listener callback
    void buttonClicked(juce::Button* button) override;

    // Layout methods
    void layoutOscillatorSection(OscillatorSection& section);
    void layoutNoiseSection();
    void layoutFilterSection();
    void layoutEnvelopeSection();
    void layoutMasterSection();
    void layoutFMSection();
    void layoutReverbSection();
    void layoutDelaySection();
    void layoutLFOSection();

    // Setup methods
    void setupComboBoxOptions();
    void createParameterAttachments();

    // Tab creation methods
    void createTabbedInterface();

    // Tab-specific setup methods
    void setupComponentStyling(juce::Component& component);
    void applyComponentStyling(juce::Component& component);
    void setupSliderForTab(juce::Slider& slider, juce::Label& label, juce::Label& valueLabel,
                          const juce::String& labelText, const juce::String& parameterID);
    void setupComboBoxForTab(juce::ComboBox& combo, juce::Label& label,
                            const juce::String& labelText, const juce::String& parameterID);
    void setupOscillatorSectionForTab(OscillatorSection& section, const juce::String& title,
                                     const juce::String& paramPrefix);
    std::unique_ptr<juce::Component> createOscillatorsTab();
    std::unique_ptr<juce::Component> createFilterEnvelopeTab();
    std::unique_ptr<juce::Component> createModulationTab();
    std::unique_ptr<juce::Component> createEffectsTab();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreOscEditor)
};