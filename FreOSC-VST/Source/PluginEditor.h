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
    - Filter section with type, cutoff, resonance
    - Envelope ADSR section
    - Effects sections: Dynamics, Reverb, Delay
    - LFO and FM synthesis sections
    - Virtual keyboard at bottom
*/
class FreOscEditor : public juce::AudioProcessorEditor, private juce::Timer, private juce::Button::Listener, private juce::ComboBox::Listener
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

    // Forward declarations
    class WaveformSelector;
    class LFOWaveformSelector;
    class OctaveSelector;
    class WaveformSelectorAttachment;
    class LFOWaveformSelectorAttachment;
    class OctaveSelectorAttachment;

    // Parameter attachments for automatic GUI updates
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<WaveformSelectorAttachment>> waveformAttachments;
    std::vector<std::unique_ptr<LFOWaveformSelectorAttachment>> lfoWaveformAttachments;
    std::vector<std::unique_ptr<OctaveSelectorAttachment>> octaveAttachments;

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

    // Custom waveform selector component
    class WaveformSelector : public juce::Component
    {
    public:
        enum Waveform { Sine = 0, Square = 1, Sawtooth = 2, Triangle = 3 };
        
        WaveformSelector();
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseMove(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
        void resized() override;
        
        void setSelectedWaveform(Waveform waveform);
        void setSelectedWaveform(int waveform) { setSelectedWaveform(static_cast<Waveform>(waveform)); }
        Waveform getSelectedWaveform() const { return selectedWaveform; }
        
        std::function<void(int)> onWaveformChanged;
        
    private:
        Waveform selectedWaveform = Sine;
        Waveform hoveredWaveform = Sine;
        bool isHovering = false;
        juce::Rectangle<int> sineButton, squareButton, sawButton, triangleButton;
        
        void drawWaveform(juce::Graphics& g, juce::Rectangle<int> area, Waveform waveform, bool isSelected);
        juce::Rectangle<int> getButtonForWaveform(Waveform waveform) const;
    };

    // Custom LFO waveform selector component - supports 5 LFO waveforms
    class LFOWaveformSelector : public juce::Component
    {
    public:
        enum LFOWaveform { LFO_Sine = 0, LFO_Triangle = 1, LFO_Sawtooth = 2, LFO_Square = 3, LFO_Random = 4 };
        
        LFOWaveformSelector();
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseMove(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
        void resized() override;
        
        void setSelectedWaveform(LFOWaveform waveform);
        void setSelectedWaveform(int waveform) { setSelectedWaveform(static_cast<LFOWaveform>(waveform)); }
        LFOWaveform getSelectedWaveform() const { return selectedWaveform; }
        
        std::function<void(int)> onWaveformChanged;
        
    private:
        LFOWaveform selectedWaveform = LFO_Sine;
        LFOWaveform hoveredWaveform = LFO_Sine;
        bool isHovering = false;
        juce::Rectangle<int> sineButton, triangleButton, sawButton, squareButton, randomButton;
        
        void drawWaveform(juce::Graphics& g, juce::Rectangle<int> area, LFOWaveform waveform, bool isSelected);
        juce::Rectangle<int> getButtonForWaveform(LFOWaveform waveform) const;
    };

    // Custom octave selector component
    class OctaveSelector : public juce::Component
    {
    public:
        enum OctaveValue { Minus2 = 0, Minus1 = 1, Zero = 2, Plus1 = 3, Plus2 = 4 };
        
        OctaveSelector();
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseMove(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
        void resized() override;
        
        void setSelectedOctave(OctaveValue octave);
        void setSelectedOctave(int octave) { setSelectedOctave(static_cast<OctaveValue>(octave)); }
        OctaveValue getSelectedOctave() const { return selectedOctave; }
        
        std::function<void(int)> onOctaveChanged;
        
    private:
        OctaveValue selectedOctave = Zero;
        OctaveValue hoveredOctave = Zero;
        bool isHovering = false;
        juce::Rectangle<int> minus2Button, minus1Button, zeroButton, plus1Button, plus2Button;
        
        void drawOctaveButton(juce::Graphics& g, juce::Rectangle<int> area, OctaveValue octave, bool isSelected);
        juce::Rectangle<int> getButtonForOctave(OctaveValue octave) const;
        juce::String getOctaveText(OctaveValue octave) const;
    };

    // Custom parameter attachment for WaveformSelector
    class WaveformSelectorAttachment : public juce::AudioProcessorValueTreeState::Listener
    {
    public:
        WaveformSelectorAttachment(juce::AudioProcessorValueTreeState& state, const juce::String& parameterID, WaveformSelector& selector);
        ~WaveformSelectorAttachment();
        
        void parameterChanged(const juce::String& parameterID, float newValue) override;
        
    private:
        void selectorChanged(int waveform);
        
        juce::AudioProcessorValueTreeState& valueTreeState;
        juce::String paramID;
        WaveformSelector& waveformSelector;
    };

    // Custom parameter attachment for LFOWaveformSelector
    class LFOWaveformSelectorAttachment : public juce::AudioProcessorValueTreeState::Listener
    {
    public:
        LFOWaveformSelectorAttachment(juce::AudioProcessorValueTreeState& state, const juce::String& parameterID, LFOWaveformSelector& selector);
        ~LFOWaveformSelectorAttachment();
        
        void parameterChanged(const juce::String& parameterID, float newValue) override;
        
    private:
        void selectorChanged(int waveform);
        
        juce::AudioProcessorValueTreeState& valueTreeState;
        juce::String paramID;
        LFOWaveformSelector& lfoWaveformSelector;
    };

    // Custom parameter attachment for OctaveSelector
    class OctaveSelectorAttachment : public juce::AudioProcessorValueTreeState::Listener
    {
    public:
        OctaveSelectorAttachment(juce::AudioProcessorValueTreeState& state, const juce::String& parameterID, OctaveSelector& selector);
        ~OctaveSelectorAttachment();
        
        void parameterChanged(const juce::String& parameterID, float newValue) override;
        
    private:
        void selectorChanged(int octave);
        
        juce::AudioProcessorValueTreeState& valueTreeState;
        juce::String paramID;
        OctaveSelector& octaveSelector;
    };

    // Oscillator sections (3 identical sets)
    
    struct OscillatorSection
    {
        juce::GroupComponent group;
        WaveformSelector waveformSelector;
        OctaveSelector octaveSelector;
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
    juce::GroupComponent filterRoutingGroup;  // Routing controls
    juce::GroupComponent filter1Group;       // Filter 1 controls
    juce::GroupComponent filter2Group;       // Filter 2 controls
    juce::ComboBox filterRoutingCombo;
    juce::Label filterRoutingLabel;
    
    // Filter 1
    juce::ComboBox filterTypeCombo;
    juce::Slider cutoffSlider, resonanceSlider, filterGainSlider;
    juce::Label filterTypeLabel, cutoffLabel, resonanceLabel, filterGainLabel;
    juce::Label cutoffValue, resonanceValue, filterGainValue;
    
    // Filter 2
    juce::ComboBox filter2TypeCombo;
    juce::Slider cutoff2Slider, resonance2Slider, filterGain2Slider;
    juce::Label filter2TypeLabel, cutoff2Label, resonance2Label, filterGain2Label;
    juce::Label cutoff2Value, resonance2Value, filterGain2Value;

    // PM synthesis section
    juce::GroupComponent pmGroup;
    juce::Slider pmIndexSlider, pmRatioSlider;
    juce::ComboBox pmCarrierCombo;
    juce::Label pmIndexLabel, pmRatioLabel, pmCarrierLabel;
    juce::Label pmIndexValue, pmRatioValue;

    // Dynamics section removed - now uses fixed internal settings

    // Effects routing section
    juce::GroupComponent effectsRoutingGroup;
    juce::ComboBox effectsRoutingCombo;
    juce::Label effectsRoutingLabel;

    // Plate Reverb section
    juce::GroupComponent reverbGroup;
    juce::Slider platePreDelaySlider, plateSizeSlider, plateDampingSlider;
    juce::Slider plateDiffusionSlider, plateWetSlider, plateWidthSlider;
    juce::Label platePreDelayLabel, plateSizeLabel, plateDampingLabel;
    juce::Label plateDiffusionLabel, plateWetLabel, plateWidthLabel;
    juce::Label platePreDelayValue, plateSizeValue, plateDampingValue;
    juce::Label plateDiffusionValue, plateWetValue, plateWidthValue;

    // Tape Delay section
    juce::GroupComponent delayGroup;
    juce::Slider tapeTimeSlider, tapeFeedbackSlider, tapeToneSlider;
    juce::Slider tapeFlutterSlider, tapeWetSlider, tapeWidthSlider;
    juce::Label tapeTimeLabel, tapeFeedbackLabel, tapeToneLabel;
    juce::Label tapeFlutterLabel, tapeWetLabel, tapeWidthLabel;
    juce::Label tapeTimeValue, tapeFeedbackValue, tapeToneValue;
    juce::Label tapeFlutterValue, tapeWetValue, tapeWidthValue;

    // Wavefolder Distortion section
    juce::GroupComponent wavefolderGroup;
    juce::Slider wavefolderDriveSlider, wavefolderThresholdSlider, wavefolderSymmetrySlider;
    juce::Slider wavefolderMixSlider, wavefolderOutputSlider;
    juce::Label wavefolderDriveLabel, wavefolderThresholdLabel, wavefolderSymmetryLabel;
    juce::Label wavefolderMixLabel, wavefolderOutputLabel;
    juce::Label wavefolderDriveValue, wavefolderThresholdValue, wavefolderSymmetryValue;
    juce::Label wavefolderMixValue, wavefolderOutputValue;

    // LFO section
    juce::GroupComponent lfoGroup;
    LFOWaveformSelector lfoWaveformSelector;
    juce::ComboBox lfoTargetCombo;
    juce::Slider lfoRateSlider, lfoAmountSlider;
    juce::Label lfoWaveformLabel, lfoTargetLabel, lfoRateLabel, lfoAmountLabel;
    juce::Label lfoRateValue, lfoAmountValue;

    // Modulation Envelope sections
    juce::GroupComponent modEnv1Group, modEnv2Group;
    juce::Slider modEnv1AttackSlider, modEnv1DecaySlider, modEnv1SustainSlider, modEnv1ReleaseSlider;
    juce::Slider modEnv2AttackSlider, modEnv2DecaySlider, modEnv2SustainSlider, modEnv2ReleaseSlider;
    juce::Slider modEnv1AmountSlider, modEnv2AmountSlider;
    juce::Slider modEnv1RateSlider, modEnv2RateSlider;
    juce::ComboBox modEnv1TargetCombo, modEnv2TargetCombo;
    juce::ComboBox modEnv1ModeCombo, modEnv2ModeCombo;
    juce::Label modEnv1AttackLabel, modEnv1DecayLabel, modEnv1SustainLabel, modEnv1ReleaseLabel;
    juce::Label modEnv2AttackLabel, modEnv2DecayLabel, modEnv2SustainLabel, modEnv2ReleaseLabel;
    juce::Label modEnv1AmountLabel, modEnv2AmountLabel, modEnv1TargetLabel, modEnv2TargetLabel;
    juce::Label modEnv1ModeLabel, modEnv2ModeLabel, modEnv1RateLabel, modEnv2RateLabel;
    juce::Label modEnv1AttackValue, modEnv1DecayValue, modEnv1SustainValue, modEnv1ReleaseValue;
    juce::Label modEnv2AttackValue, modEnv2DecayValue, modEnv2SustainValue, modEnv2ReleaseValue;
    juce::Label modEnv1AmountValue, modEnv2AmountValue, modEnv1RateValue, modEnv2RateValue;

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
    juce::String formatPMRatioValue(float ratioValue);
    juce::String formatMasterVolumeValue(float normalizedValue);

    // Parameter update callbacks
    void updateValueLabels();
    void updateScaledFonts();

    // Timer callback for real-time updates
    void timerCallback() override;

    // Button listener callback
    void buttonClicked(juce::Button* button) override;
    
    // ComboBox listener callback
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

    // Layout methods
    void layoutOscillatorSection(OscillatorSection& section);
    void layoutNoiseSection();
    // layoutFilterSection() removed - now using GroupComponent layouts in tabs
    void layoutEnvelopeSection();
    void layoutMasterSection();
    void layoutPMSection();
    void layoutReverbSection();
    void layoutDelaySection();
    void layoutLFOSection();

    // Setup methods
    void setupComboBoxOptions();
    void createParameterAttachments();

    // Tab creation methods
    void createTabbedInterface();

    // Tab-specific setup methods
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