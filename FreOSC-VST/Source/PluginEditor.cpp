#include "PluginEditor.h"

//==============================================================================
// FreOSC blue theme - elegant blue gradient interface
const juce::Colour FreOscEditor::backgroundColour = juce::Colour(0xff2d4a87);  // Deep blue background
const juce::Colour FreOscEditor::panelColour = juce::Colour(0xff3e5a99);       // Lighter blue panels
const juce::Colour FreOscEditor::accentColour = juce::Colour(0xff5ba3d0);      // Light blue accent
const juce::Colour FreOscEditor::textColour = juce::Colour(0xffc4d6ee);        // Light blue-grey text
const juce::Colour FreOscEditor::knobColour = juce::Colour(0xff1e3a5f);        // Dark blue for controls

//==============================================================================
// Custom LookAndFeel to remove tab gradients
class FreOscLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver, bool isMouseDown) override
    {
        juce::ignoreUnused(isMouseOver, isMouseDown);
        
        const auto area = button.getActiveArea();
        const auto isActive = button.isFrontTab();
        
        // Use solid colors instead of gradients
        if (isActive)
        {
            g.setColour(FreOscEditor::backgroundColour.brighter(0.1f));
        }
        else
        {
            g.setColour(FreOscEditor::panelColour);
        }
        
        g.fillRect(area);
        
        // Draw outline
        g.setColour(FreOscEditor::knobColour);
        g.drawRect(area, 1);
        
        // Draw text
        g.setColour(FreOscEditor::textColour);
        g.setFont(14.0f);
        g.drawText(button.getButtonText(), area, juce::Justification::centred);
    }
};

//==============================================================================
FreOscEditor::FreOscEditor (FreOscProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), tabbedComponent(juce::TabbedButtonBar::TabsAtTop)
{
    // Set editor size to match web interface proportions
    setSize(1400, 900);

    // Make the editor resizable with constraints
    setResizable(true, true);

    // Set minimum and maximum size constraints
    // Minimum size where controls remain usable, then becomes scrollable
    // Increased to prevent knob overlap with labels - triggers scrolling earlier
    minUsableWidth = 950;   // Minimum width where controls are still usable (increased from 800)
    minUsableHeight = 700;  // Minimum height where controls are still usable (increased from 600)

    getConstrainer()->setMinimumSize(400, 300);   // Absolute minimum (will scroll)
    getConstrainer()->setMaximumSize(2100, 1350); // 150% maximum

    // Don't maintain fixed aspect ratio to allow more flexible resizing
    // getConstrainer()->setFixedAspectRatio(14.0 / 9.0);

    // Setup scrollable viewport system
    viewport.setViewedComponent(&contentComponent, false);
    viewport.setScrollBarsShown(true, true);
    addAndMakeVisible(viewport);

    // Setup header section (goes in content component)
    setupComponent(titleLabel);
    titleLabel.setText("FreOSC", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions().withHeight(20.0f * currentScale)));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    setupComponent(presetSelector);
    setupComponent(loadPresetButton);
    loadPresetButton.setButtonText("Load Preset");
    loadPresetButton.addListener(this);

    // Add header components to content component instead of main editor
    contentComponent.addAndMakeVisible(titleLabel);
    contentComponent.addAndMakeVisible(presetSelector);
    contentComponent.addAndMakeVisible(loadPresetButton);

    // DO NOT setup components in main constructor - let tabs handle this
    // This avoids the component parent conflict issue
    // NOTE: Combo box options will be set up by individual tabs

    // Create custom look and feel for solid tabs
    customLookAndFeel = std::make_unique<FreOscLookAndFeel>();
    
    // Create tabbed interface
    createTabbedInterface();

    // Setup combo box options (including presets)
    setupComboBoxOptions();

    // Setup parameter attachments for automatic GUI updates
    createParameterAttachments();

    // Start timer for value label updates
    startTimerHz(30); // 30 FPS update rate
}

FreOscEditor::~FreOscEditor()
{
    // Clean up custom look and feel
    if (customLookAndFeel)
    {
        tabbedComponent.getTabbedButtonBar().setLookAndFeel(nullptr);
    }
    stopTimer();
}

//==============================================================================
void FreOscEditor::paint (juce::Graphics& g)
{
    // FreOSC solid blue background - no gradients
    auto bounds = getLocalBounds();
    g.setColour(backgroundColour);
    g.fillRect(bounds);
}

void FreOscEditor::resized()
{
    auto bounds = getLocalBounds();

    // Always fill the entire editor with the viewport
    viewport.setBounds(bounds);

    // Calculate if we need scrolling based on current size vs minimum usable size
    bool needsScrolling = getWidth() < minUsableWidth || getHeight() < minUsableHeight;

    // Set content component size - either fit the viewport or use minimum usable size
    int contentWidth = juce::jmax(getWidth(), minUsableWidth);
    int contentHeight = juce::jmax(getHeight(), minUsableHeight);

    contentComponent.setSize(contentWidth, contentHeight);

    // Calculate scaling factors based on content size vs original size (1400x900)
    float scaleX = contentWidth / 1400.0f;
    float scaleY = contentHeight / 900.0f;
    float scale = juce::jmin(scaleX, scaleY); // Use minimum to maintain proportions

    // Store scale for use in other methods
    currentScale = scale;

    // Update fonts for scaling
    updateScaledFonts();

    // Layout components in content component
    auto contentBounds = contentComponent.getLocalBounds();

    // Header section (compact)
    auto headerBounds = contentBounds.removeFromTop(50);
    titleLabel.setBounds(headerBounds.removeFromLeft(300).reduced(5));
    auto presetBounds = headerBounds.reduced(5);
    presetSelector.setBounds(presetBounds.removeFromLeft(200));
    loadPresetButton.setBounds(presetBounds.removeFromLeft(100).withTrimmedLeft(10));

    // Main area for tabs
    auto mainArea = contentBounds.reduced(10);
    tabbedComponent.setBounds(mainArea);

    // Show/hide scrollbars based on whether we need scrolling
    viewport.setScrollBarsShown(needsScrolling, needsScrolling);
}

void FreOscEditor::timerCallback()
{
    updateValueLabels();
}

void FreOscEditor::buttonClicked(juce::Button* button)
{
    if (button == &loadPresetButton)
    {
        // Get selected preset from combo box
        int selectedIndex = presetSelector.getSelectedItemIndex();
        if (selectedIndex > 0) // Index 0 is "Custom", start from 1
        {
            // Load the preset (selectedIndex - 1 because index 0 is "Custom", so preset 0 is at index 1)
            audioProcessor.loadPreset(selectedIndex - 1);
        }
    }
}

//==============================================================================
// Helper method implementations
void FreOscEditor::setupComponent(juce::Component& component)
{
    addAndMakeVisible(component);

    // Apply FL Studio 3x Osc inspired styling - metallic grey look
    if (auto* label = dynamic_cast<juce::Label*>(&component))
    {
        label->setColour(juce::Label::textColourId, textColour);
        label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label->setJustificationType(juce::Justification::centred);
    }
    else if (auto* slider = dynamic_cast<juce::Slider*>(&component))
    {
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        // Metallic knob styling like FL Studio
        slider->setColour(juce::Slider::rotarySliderFillColourId, accentColour);
        slider->setColour(juce::Slider::rotarySliderOutlineColourId, knobColour.darker(0.3f));
        slider->setColour(juce::Slider::thumbColourId, juce::Colour(0xff87ceeb)); // Light blue thumb
        slider->setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    }
    else if (auto* combo = dynamic_cast<juce::ComboBox*>(&component))
    {
        combo->setColour(juce::ComboBox::backgroundColourId, panelColour.brighter(0.1f));
        combo->setColour(juce::ComboBox::textColourId, textColour);
        combo->setColour(juce::ComboBox::outlineColourId, knobColour);
        combo->setColour(juce::ComboBox::arrowColourId, textColour);
        combo->setColour(juce::ComboBox::buttonColourId, panelColour);
    }
    else if (auto* button = dynamic_cast<juce::TextButton*>(&component))
    {
        button->setColour(juce::TextButton::buttonColourId, panelColour);
        button->setColour(juce::TextButton::buttonOnColourId, accentColour);
        button->setColour(juce::TextButton::textColourOffId, textColour);
        button->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    }
    else if (auto* group = dynamic_cast<juce::GroupComponent*>(&component))
    {
        group->setColour(juce::GroupComponent::outlineColourId, knobColour);
        group->setColour(juce::GroupComponent::textColourId, textColour);
    }
}

void FreOscEditor::setupSlider(juce::Slider& slider, juce::Label& label, juce::Label& valueLabel,
                              const juce::String& labelText, const juce::String& parameterID)
{
    setupComponent(slider);
    setupComponent(label);
    setupComponent(valueLabel);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions().withHeight(12.0f * currentScale)));
    label.setJustificationType(juce::Justification::centred);

    valueLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f * currentScale)));
    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setColour(juce::Label::textColourId, accentColour.brighter(0.1f)); // Light blue value display
    valueLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    // Set parameter range from processor
    auto* param = audioProcessor.getValueTreeState().getParameter(parameterID);
    if (param != nullptr)
    {
        auto range = audioProcessor.getValueTreeState().getParameterRange(parameterID);
        slider.setRange(range.start, range.end, range.interval);
        slider.setValue(param->getValue() * (range.end - range.start) + range.start, juce::dontSendNotification);
    }
}

void FreOscEditor::setupComboBox(juce::ComboBox& combo, juce::Label& label,
                                const juce::String& labelText, const juce::String& parameterID)
{
    setupComponent(combo);
    setupComponent(label);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions().withHeight(12.0f * currentScale)));
    label.setJustificationType(juce::Justification::centred);

    // Parameter ID is available for future parameter setup if needed
    juce::ignoreUnused(parameterID);
}

void FreOscEditor::setupOscillatorSection(OscillatorSection& section, const juce::String& title,
                                         const juce::String& paramPrefix)
{
    setupComponent(section.group);
    section.group.setText(title);

    setupComboBox(section.waveformCombo, section.waveformLabel, "Waveform:", paramPrefix + "waveform");
    setupComboBox(section.octaveCombo, section.octaveLabel, "Octave:", paramPrefix + "octave");
    setupSlider(section.levelSlider, section.levelLabel, section.levelValue, "Level:", paramPrefix + "level");
    setupSlider(section.detuneSlider, section.detuneLabel, section.detuneValue, "Detune:", paramPrefix + "detune");
    setupSlider(section.panSlider, section.panLabel, section.panValue, "Pan:", paramPrefix + "pan");
}

juce::Rectangle<int> FreOscEditor::getSectionBounds(int row, int col, int width, int height)
{
    auto sectionWidth = getWidth() / 4;
    auto sectionHeight = (getHeight() - 80) / 4; // 80px for header

    return juce::Rectangle<int>(
        col * sectionWidth,
        80 + row * sectionHeight,
        width * sectionWidth,
        height * sectionHeight
    ).reduced(5); // 5px padding
}

void FreOscEditor::layoutOscillatorSection(OscillatorSection& section)
{
    auto bounds = section.group.getBounds().reduced(8);

    // More organized layout like the web interface
    auto topRow = bounds.removeFromTop(30);

    // Top row: Waveform and Octave side by side
    auto waveformArea = topRow.removeFromLeft(topRow.getWidth() / 2).reduced(2);
    section.waveformLabel.setBounds(waveformArea.removeFromTop(12));
    section.waveformCombo.setBounds(waveformArea);

    auto octaveArea = topRow.reduced(2);
    section.octaveLabel.setBounds(octaveArea.removeFromTop(12));
    section.octaveCombo.setBounds(octaveArea);

    // Bottom area: Level, Detune, Pan as sliders
    auto controlsArea = bounds.withTrimmedTop(5);
    auto sliderWidth = controlsArea.getWidth() / 3;

    // Level
    auto levelArea = controlsArea.removeFromLeft(sliderWidth).reduced(3);
    section.levelLabel.setBounds(levelArea.removeFromTop(12));
    section.levelSlider.setBounds(levelArea.removeFromTop(levelArea.getHeight() - 12));
    section.levelValue.setBounds(levelArea);

    // Detune
    auto detuneArea = controlsArea.removeFromLeft(sliderWidth).reduced(3);
    section.detuneLabel.setBounds(detuneArea.removeFromTop(12));
    section.detuneSlider.setBounds(detuneArea.removeFromTop(detuneArea.getHeight() - 12));
    section.detuneValue.setBounds(detuneArea);

    // Pan
    auto panArea = controlsArea.reduced(3);
    section.panLabel.setBounds(panArea.removeFromTop(12));
    section.panSlider.setBounds(panArea.removeFromTop(panArea.getHeight() - 12));
    section.panValue.setBounds(panArea);
}

void FreOscEditor::layoutNoiseSection()
{
    auto bounds = noiseGroup.getBounds().reduced(10);
    auto rowHeight = bounds.getHeight() / 4;

    // Type
    auto row = bounds.removeFromTop(rowHeight);
    noiseTypeLabel.setBounds(row.removeFromLeft(60));
    noiseTypeCombo.setBounds(row);

    // Level
    row = bounds.removeFromTop(rowHeight);
    noiseLevelLabel.setBounds(row.removeFromTop(15));
    noiseLevelSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    noiseLevelValue.setBounds(row);

    // Pan
    row = bounds.removeFromTop(rowHeight);
    noisePanLabel.setBounds(row.removeFromTop(15));
    noisePanSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    noisePanValue.setBounds(row);
}

void FreOscEditor::layoutFilterSection()
{
    auto bounds = filterGroup.getBounds().reduced(10);
    auto rowHeight = bounds.getHeight() / 6;

    // Type
    auto row = bounds.removeFromTop(rowHeight);
    filterTypeLabel.setBounds(row.removeFromLeft(60));
    filterTypeCombo.setBounds(row);

    // Cutoff
    row = bounds.removeFromTop(rowHeight);
    cutoffLabel.setBounds(row.removeFromTop(15));
    cutoffSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    cutoffValue.setBounds(row);

    // Resonance
    row = bounds.removeFromTop(rowHeight);
    resonanceLabel.setBounds(row.removeFromTop(15));
    resonanceSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    resonanceValue.setBounds(row);

    // Gain
    row = bounds.removeFromTop(rowHeight);
    filterGainLabel.setBounds(row.removeFromTop(15));
    filterGainSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    filterGainValue.setBounds(row);

    // Formant vowel
    row = bounds.removeFromTop(rowHeight);
    formantLabel.setBounds(row.removeFromLeft(60));
    formantVowelCombo.setBounds(row);
}

void FreOscEditor::layoutEnvelopeSection()
{
    auto bounds = envelopeGroup.getBounds().reduced(10);
    auto colWidth = bounds.getWidth() / 2;
    auto rowHeight = bounds.getHeight() / 2;

    // Attack (top-left)
    auto section = juce::Rectangle<int>(bounds.getX(), bounds.getY(), colWidth, rowHeight).reduced(2);
    attackLabel.setBounds(section.removeFromTop(15));
    attackSlider.setBounds(section.removeFromTop(section.getHeight() - 15));
    attackValue.setBounds(section);

    // Decay (top-right)
    section = juce::Rectangle<int>(bounds.getX() + colWidth, bounds.getY(), colWidth, rowHeight).reduced(2);
    decayLabel.setBounds(section.removeFromTop(15));
    decaySlider.setBounds(section.removeFromTop(section.getHeight() - 15));
    decayValue.setBounds(section);

    // Sustain (bottom-left)
    section = juce::Rectangle<int>(bounds.getX(), bounds.getY() + rowHeight, colWidth, rowHeight).reduced(2);
    sustainLabel.setBounds(section.removeFromTop(15));
    sustainSlider.setBounds(section.removeFromTop(section.getHeight() - 15));
    sustainValue.setBounds(section);

    // Release (bottom-right)
    section = juce::Rectangle<int>(bounds.getX() + colWidth, bounds.getY() + rowHeight, colWidth, rowHeight).reduced(2);
    releaseLabel.setBounds(section.removeFromTop(15));
    releaseSlider.setBounds(section.removeFromTop(section.getHeight() - 15));
    releaseValue.setBounds(section);
}

void FreOscEditor::layoutMasterSection()
{
    auto bounds = masterGroup.getBounds().reduced(10);

    masterVolumeLabel.setBounds(bounds.removeFromTop(20));
    masterVolumeSlider.setBounds(bounds.removeFromTop(bounds.getHeight() - 20));
    masterVolumeValue.setBounds(bounds);
}

void FreOscEditor::layoutFMSection()
{
    auto bounds = fmGroup.getBounds().reduced(10);
    auto rowHeight = bounds.getHeight() / 4; // Updated to 4 rows since we removed source

    // Amount
    auto row = bounds.removeFromTop(rowHeight);
    fmAmountLabel.setBounds(row.removeFromTop(15));
    fmAmountSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    fmAmountValue.setBounds(row);

    // Ratio
    row = bounds.removeFromTop(rowHeight);
    fmRatioLabel.setBounds(row.removeFromTop(15));
    fmRatioSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    fmRatioValue.setBounds(row);

    // Target (Source is always Oscillator 3)
    row = bounds.removeFromTop(rowHeight);
    fmTargetLabel.setBounds(row.removeFromLeft(60));
    fmTargetCombo.setBounds(row);
}


void FreOscEditor::layoutReverbSection()
{
    auto bounds = reverbGroup.getBounds().reduced(10);
    auto rowHeight = bounds.getHeight() / 2;

    // Room Size
    auto row = bounds.removeFromTop(rowHeight);
    roomSizeLabel.setBounds(row.removeFromTop(15));
    roomSizeSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    roomSizeValue.setBounds(row);

    // Wet Level
    row = bounds;
    reverbWetLabel.setBounds(row.removeFromTop(15));
    reverbWetSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    reverbWetValue.setBounds(row);
}

void FreOscEditor::layoutDelaySection()
{
    auto bounds = delayGroup.getBounds().reduced(10);
    auto rowHeight = bounds.getHeight() / 3;

    // Time
    auto row = bounds.removeFromTop(rowHeight);
    delayTimeLabel.setBounds(row.removeFromTop(15));
    delayTimeSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    delayTimeValue.setBounds(row);

    // Feedback
    row = bounds.removeFromTop(rowHeight);
    delayFeedbackLabel.setBounds(row.removeFromTop(15));
    delayFeedbackSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    delayFeedbackValue.setBounds(row);

    // Wet Level
    row = bounds;
    delayWetLabel.setBounds(row.removeFromTop(15));
    delayWetSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    delayWetValue.setBounds(row);
}

void FreOscEditor::layoutLFOSection()
{
    auto bounds = lfoGroup.getBounds().reduced(10);
    auto rowHeight = bounds.getHeight() / 5;

    // Waveform
    auto row = bounds.removeFromTop(rowHeight);
    lfoWaveformLabel.setBounds(row.removeFromLeft(60));
    lfoWaveformCombo.setBounds(row);

    // Target
    row = bounds.removeFromTop(rowHeight);
    lfoTargetLabel.setBounds(row.removeFromLeft(60));
    lfoTargetCombo.setBounds(row);

    // Rate
    row = bounds.removeFromTop(rowHeight);
    lfoRateLabel.setBounds(row.removeFromTop(15));
    lfoRateSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    lfoRateValue.setBounds(row);

    // Amount
    row = bounds;
    lfoAmountLabel.setBounds(row.removeFromTop(15));
    lfoAmountSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    lfoAmountValue.setBounds(row);
}

void FreOscEditor::setupComboBoxOptions()
{
    // Waveform options for oscillators
    for (auto& section : {&osc1Section, &osc2Section, &osc3Section})
    {
        section->waveformCombo.addItem("Sine", 1);
        section->waveformCombo.addItem("Square", 2);
        section->waveformCombo.addItem("Sawtooth", 3);
        section->waveformCombo.addItem("Triangle", 4);
    }

    // Octave options for oscillators
    for (auto& section : {&osc1Section, &osc2Section, &osc3Section})
    {
        section->octaveCombo.addItem("-2", 1);
        section->octaveCombo.addItem("-1", 2);
        section->octaveCombo.addItem("0", 3);
        section->octaveCombo.addItem("+1", 4);
        section->octaveCombo.addItem("+2", 5);
    }

    // Noise type options
    noiseTypeCombo.addItem("White Noise", 1);
    noiseTypeCombo.addItem("Pink Noise", 2);
    noiseTypeCombo.addItem("Brown Noise", 3);
    noiseTypeCombo.addItem("Blue Noise", 4);
    noiseTypeCombo.addItem("Violet Noise", 5);
    noiseTypeCombo.addItem("Grey Noise", 6);
    noiseTypeCombo.addItem("Vinyl Crackle", 7);
    noiseTypeCombo.addItem("Digital Noise", 8);
    noiseTypeCombo.addItem("Wind Noise", 9);
    noiseTypeCombo.addItem("Ocean Waves", 10);

    // Filter type options
    filterTypeCombo.addItem("Low Pass", 1);
    filterTypeCombo.addItem("High Pass", 2);
    filterTypeCombo.addItem("Band Pass", 3);
    filterTypeCombo.addItem("Low Shelf", 4);
    filterTypeCombo.addItem("High Shelf", 5);
    filterTypeCombo.addItem("Peaking", 6);
    filterTypeCombo.addItem("Notch", 7);
    filterTypeCombo.addItem("All Pass", 8);
    filterTypeCombo.addItem("Formant", 9);

    // Formant vowel options
    formantVowelCombo.addItem("A (ah)", 1);
    formantVowelCombo.addItem("E (eh)", 2);
    formantVowelCombo.addItem("I (ee)", 3);
    formantVowelCombo.addItem("O (oh)", 4);
    formantVowelCombo.addItem("U (oo)", 5);
    formantVowelCombo.addItem("AE (ay)", 6);
    formantVowelCombo.addItem("AW (aw)", 7);
    formantVowelCombo.addItem("ER (ur)", 8);

    // FM source is always Oscillator 3 - no combo box needed

    // FM target options - updated for new routing
    fmTargetCombo.addItem("Oscillator 1", 1);
    fmTargetCombo.addItem("Oscillator 2", 2);
    fmTargetCombo.addItem("Both Osc 1 & 2", 3);

    // LFO waveform options
    lfoWaveformCombo.addItem("Sine", 1);
    lfoWaveformCombo.addItem("Triangle", 2);
    lfoWaveformCombo.addItem("Sawtooth", 3);
    lfoWaveformCombo.addItem("Square", 4);
    lfoWaveformCombo.addItem("Random", 5);

    // LFO target options
    lfoTargetCombo.addItem("None", 1);
    lfoTargetCombo.addItem("Pitch", 2);
    lfoTargetCombo.addItem("Filter Cutoff", 3);
    lfoTargetCombo.addItem("Volume", 4);
    lfoTargetCombo.addItem("Pan", 5);

    // Preset options - get from JSON preset manager
    presetSelector.addItem("Custom", 1); // Always have "Custom" as first option

    // Get preset names from the new JSON preset manager
    auto presetNames = audioProcessor.getPresets().getPresetNames();
    for (int i = 0; i < presetNames.size(); ++i)
    {
        juce::String displayName = presetNames[i];
        // Remove "Factory_" prefix for cleaner display
        if (displayName.startsWith("Factory_"))
            displayName = displayName.substring(8);
        // Replace underscores with spaces
        displayName = displayName.replace("_", " ");

        presetSelector.addItem(displayName, i + 2); // Start from ID 2 (after "Custom")
    }

    // Set default selection to "Custom"
    presetSelector.setSelectedId(1, juce::dontSendNotification);
}

void FreOscEditor::createParameterAttachments()
{
    auto& valueTreeState = audioProcessor.getValueTreeState();

    // Create slider attachments
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "osc1_level", osc1Section.levelSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "osc1_detune", osc1Section.detuneSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "osc1_pan", osc1Section.panSlider));

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "osc2_level", osc2Section.levelSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "osc2_detune", osc2Section.detuneSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "osc2_pan", osc2Section.panSlider));

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "osc3_level", osc3Section.levelSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "osc3_detune", osc3Section.detuneSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "osc3_pan", osc3Section.panSlider));

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "noise_level", noiseLevelSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "noise_pan", noisePanSlider));

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "master_volume", masterVolumeSlider));

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "envelope_attack", attackSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "envelope_decay", decaySlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "envelope_sustain", sustainSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "envelope_release", releaseSlider));

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "filter_cutoff", cutoffSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "filter_resonance", resonanceSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "filter_gain", filterGainSlider));

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "fm_amount", fmAmountSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "fm_ratio", fmRatioSlider));

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "lfo_rate", lfoRateSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "lfo_amount", lfoAmountSlider));

    // Dynamics parameters removed - now uses fixed internal settings

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "reverb_room_size", roomSizeSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "reverb_wet_level", reverbWetSlider));

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "delay_time", delayTimeSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "delay_feedback", delayFeedbackSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "delay_wet_level", delayWetSlider));

    // Create combo box attachments
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "osc1_waveform", osc1Section.waveformCombo));
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "osc1_octave", osc1Section.octaveCombo));

    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "osc2_waveform", osc2Section.waveformCombo));
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "osc2_octave", osc2Section.octaveCombo));

    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "osc3_waveform", osc3Section.waveformCombo));
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "osc3_octave", osc3Section.octaveCombo));

    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "noise_type", noiseTypeCombo));

    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "filter_type", filterTypeCombo));
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "formant_vowel", formantVowelCombo));

    // FM source is always Osc3 - no parameter attachment needed
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "fm_target", fmTargetCombo));

    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "lfo_waveform", lfoWaveformCombo));
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "lfo_target", lfoTargetCombo));
}

void FreOscEditor::updateValueLabels()
{
    // Update oscillator value labels
    osc1Section.levelValue.setText(juce::String(osc1Section.levelSlider.getValue(), 2), juce::dontSendNotification);
    osc1Section.detuneValue.setText(juce::String(static_cast<int>(osc1Section.detuneSlider.getValue())) + "¢", juce::dontSendNotification);
    osc1Section.panValue.setText(formatPanValue(static_cast<float>(osc1Section.panSlider.getValue())), juce::dontSendNotification);

    osc2Section.levelValue.setText(juce::String(osc2Section.levelSlider.getValue(), 2), juce::dontSendNotification);
    osc2Section.detuneValue.setText(juce::String(static_cast<int>(osc2Section.detuneSlider.getValue())) + "¢", juce::dontSendNotification);
    osc2Section.panValue.setText(formatPanValue(static_cast<float>(osc2Section.panSlider.getValue())), juce::dontSendNotification);

    osc3Section.levelValue.setText(juce::String(osc3Section.levelSlider.getValue(), 2), juce::dontSendNotification);
    osc3Section.detuneValue.setText(juce::String(static_cast<int>(osc3Section.detuneSlider.getValue())) + "¢", juce::dontSendNotification);
    osc3Section.panValue.setText(formatPanValue(static_cast<float>(osc3Section.panSlider.getValue())), juce::dontSendNotification);

    // Update other value labels
    noiseLevelValue.setText(juce::String(noiseLevelSlider.getValue(), 2), juce::dontSendNotification);
    noisePanValue.setText(formatPanValue(static_cast<float>(noisePanSlider.getValue())), juce::dontSendNotification);

    masterVolumeValue.setText(juce::String(masterVolumeSlider.getValue(), 2), juce::dontSendNotification);

    attackValue.setText(formatTimeValue(static_cast<float>(attackSlider.getValue())), juce::dontSendNotification);
    decayValue.setText(formatTimeValue(static_cast<float>(decaySlider.getValue())), juce::dontSendNotification);
    sustainValue.setText(juce::String(sustainSlider.getValue(), 2), juce::dontSendNotification);
    releaseValue.setText(formatTimeValue(static_cast<float>(releaseSlider.getValue())), juce::dontSendNotification);

    cutoffValue.setText(formatFrequencyValue(static_cast<float>(cutoffSlider.getValue())), juce::dontSendNotification);
    resonanceValue.setText(formatResonanceValue(static_cast<float>(resonanceSlider.getValue())), juce::dontSendNotification);
    filterGainValue.setText(formatFilterGainValue(static_cast<float>(filterGainSlider.getValue())), juce::dontSendNotification);

    fmAmountValue.setText(juce::String(static_cast<int>(fmAmountSlider.getValue())), juce::dontSendNotification);
    fmRatioValue.setText(juce::String(fmRatioSlider.getValue(), 1), juce::dontSendNotification);

    // Dynamics removed from user control - values updated automatically with fixed settings

    roomSizeValue.setText(juce::String(roomSizeSlider.getValue(), 2), juce::dontSendNotification);
    reverbWetValue.setText(juce::String(reverbWetSlider.getValue(), 2), juce::dontSendNotification);

    delayTimeValue.setText(juce::String(static_cast<int>(delayTimeSlider.getValue())) + "ms", juce::dontSendNotification);
    delayFeedbackValue.setText(juce::String(delayFeedbackSlider.getValue(), 2), juce::dontSendNotification);
    delayWetValue.setText(juce::String(delayWetSlider.getValue(), 2), juce::dontSendNotification);

    lfoRateValue.setText(juce::String(lfoRateSlider.getValue(), 2) + " Hz", juce::dontSendNotification);
    lfoAmountValue.setText(juce::String(lfoAmountSlider.getValue(), 2), juce::dontSendNotification);
}

juce::String FreOscEditor::formatPanValue(float value)
{
    if (std::abs(value) < 0.01f)
        return "Center";
    else if (value < 0.0f)
        return "L" + juce::String(static_cast<int>(std::abs(value) * 100.0f)) + "%";
    else
        return "R" + juce::String(static_cast<int>(value * 100.0f)) + "%";
}

juce::String FreOscEditor::formatTimeValue(float value)
{
    if (value < 0.01f) return juce::String(static_cast<int>(value * 1000.0f)) + "ms";
    return juce::String(value, 2) + "s";
}

juce::String FreOscEditor::formatFrequencyValue(float normalizedValue)
{
    // Convert normalized value (0.0-1.0) to frequency (20Hz-20kHz)
    // This matches FreOscFilter::normalizedToFrequency()
    float freq = 20.0f * std::pow(1000.0f, juce::jlimit(0.0f, 1.0f, normalizedValue));

    if (freq < 1000.0f) return juce::String(static_cast<int>(freq)) + "Hz";
    return juce::String(freq / 1000.0f, 1) + "kHz";
}

juce::String FreOscEditor::formatResonanceValue(float normalizedValue)
{
    // Convert normalized value (0.0-1.0) to Q (0.1-30.0)
    // This matches FreOscFilter::normalizedToQ()
    float q = 0.1f + (juce::jlimit(0.0f, 1.0f, normalizedValue) * 29.9f);
    return juce::String(q, 1);
}

juce::String FreOscEditor::formatFilterGainValue(float normalizedValue)
{
    // Convert normalized value (0.0-1.0) to dB (-24dB to +24dB)
    // This matches FreOscFilter::normalizedToGainDb()
    float gainDb = -24.0f + (juce::jlimit(0.0f, 1.0f, normalizedValue) * 48.0f);
    return juce::String(gainDb, 1) + "dB";
}

void FreOscEditor::updateScaledFonts()
{
    // Update title label font
    titleLabel.setFont(juce::Font(juce::FontOptions().withHeight(20.0f * currentScale)));

    // Update all label fonts throughout the interface
    // Note: In a more complex implementation, we would iterate through all labels
    // For now, fonts will update as components are laid out
}

//==============================================================================
// Tab Implementation Methods

void FreOscEditor::createTabbedInterface()
{
    // Setup tabbed component with FL Studio metallic theme
    tabbedComponent.setTabBarDepth(28);
    tabbedComponent.setOutline(1);
    tabbedComponent.setColour(juce::TabbedComponent::backgroundColourId, backgroundColour);
    tabbedComponent.setColour(juce::TabbedComponent::outlineColourId, knobColour);

    // Apply custom look and feel to remove gradients
    auto& tabBar = tabbedComponent.getTabbedButtonBar();
    tabBar.setLookAndFeel(customLookAndFeel.get());

    // Create tab content
    oscillatorsTab = createOscillatorsTab();
    filterEnvelopeTab = createFilterEnvelopeTab();
    modulationTab = createModulationTab();
    effectsTab = createEffectsTab();
    masterTab = createMasterTab();

    // Add tabs with metallic grey theme
    tabbedComponent.addTab("OSC", panelColour, oscillatorsTab.get(), false);
    tabbedComponent.addTab("FILTER", panelColour, filterEnvelopeTab.get(), false);
    tabbedComponent.addTab("MOD", panelColour, modulationTab.get(), false);
    tabbedComponent.addTab("EFFECTS", panelColour, effectsTab.get(), false);
    tabbedComponent.addTab("MASTER", panelColour, masterTab.get(), false);

    // Make visible and add to content component (not main editor)
    contentComponent.addAndMakeVisible(tabbedComponent);
}

std::unique_ptr<juce::Component> FreOscEditor::createOscillatorsTab()
{
    class OscillatorsTabComponent : public juce::Component
    {
    public:
        OscillatorsTabComponent(FreOscEditor& editor) : owner(editor)
        {
            // Direct component layout solution implemented - no more test components needed

            // SOLUTION: Use direct component layout instead of GroupComponent
            // This avoids the coordinate system conflict that was breaking osc2/osc3

            // Setup ALL oscillators with DIRECT component layout (no GroupComponent)
            // This avoids the coordinate system conflict for all oscillators
            setupDirectOscillator(owner.osc1Section, "Oscillator 1", "osc1_");
            setupDirectOscillator(owner.osc2Section, "Oscillator 2", "osc2_");
            setupDirectOscillator(owner.osc3Section, "Oscillator 3", "osc3_");

            // Setup noise generator with direct layout
            setupDirectNoise();
        }

        void setupDirectOscillator(FreOscEditor::OscillatorSection& section, const juce::String& title, [[maybe_unused]] const juce::String& paramPrefix)
        {
            // DIRECT SETUP: Add components directly to tab (no GroupComponent)
            // This avoids coordinate system conflicts

            // Setup components with proper styling
            owner.applyComponentStyling(section.waveformCombo);
            owner.applyComponentStyling(section.octaveCombo);
            owner.applyComponentStyling(section.levelSlider);
            owner.applyComponentStyling(section.detuneSlider);
            owner.applyComponentStyling(section.panSlider);

            owner.applyComponentStyling(section.waveformLabel);
            owner.applyComponentStyling(section.octaveLabel);
            owner.applyComponentStyling(section.levelLabel);
            owner.applyComponentStyling(section.detuneLabel);
            owner.applyComponentStyling(section.panLabel);

            // CRITICAL: Apply proper styling to VALUE LABELS like noise generator
            owner.applyComponentStyling(section.levelValue);
            owner.applyComponentStyling(section.detuneValue);
            owner.applyComponentStyling(section.panValue);

            // Set value label styling to match noise generator (orange accent color)
            section.levelValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            section.levelValue.setJustificationType(juce::Justification::centred);
            section.levelValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            section.levelValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            section.detuneValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            section.detuneValue.setJustificationType(juce::Justification::centred);
            section.detuneValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            section.detuneValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            section.panValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            section.panValue.setJustificationType(juce::Justification::centred);
            section.panValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            section.panValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // Combo box options are set up in setupComboBoxOptions() - no need to duplicate here

            // Set labels
            section.waveformLabel.setText("Waveform:", juce::dontSendNotification);
            section.octaveLabel.setText("Octave:", juce::dontSendNotification);
            section.levelLabel.setText("Level:", juce::dontSendNotification);
            section.detuneLabel.setText("Detune:", juce::dontSendNotification);
            section.panLabel.setText("Pan:", juce::dontSendNotification);

            // Add ALL components directly to the tab (not to a group) - INCLUDING VALUE LABELS
            addAndMakeVisible(section.waveformLabel);
            addAndMakeVisible(section.waveformCombo);
            addAndMakeVisible(section.octaveLabel);
            addAndMakeVisible(section.octaveCombo);
            addAndMakeVisible(section.levelLabel);
            addAndMakeVisible(section.levelSlider);
            addAndMakeVisible(section.levelValue);  // CRITICAL: Add value label to hierarchy
            addAndMakeVisible(section.detuneLabel);
            addAndMakeVisible(section.detuneSlider);
            addAndMakeVisible(section.detuneValue); // CRITICAL: Add value label to hierarchy
            addAndMakeVisible(section.panLabel);
            addAndMakeVisible(section.panSlider);
            addAndMakeVisible(section.panValue);    // CRITICAL: Add value label to hierarchy

            // Store section info for layout
            if (title.contains("2")) osc2SectionBounds = true;
            if (title.contains("3")) osc3SectionBounds = true;
        }

        void setupDirectNoise()
        {
            // DIRECT SETUP: Add noise components directly to tab (no GroupComponent)

            // Setup components with proper styling
            owner.applyComponentStyling(owner.noiseTypeCombo);
            owner.applyComponentStyling(owner.noiseLevelSlider);
            owner.applyComponentStyling(owner.noisePanSlider);

            owner.applyComponentStyling(owner.noiseTypeLabel);
            owner.applyComponentStyling(owner.noiseLevelLabel);
            owner.applyComponentStyling(owner.noisePanLabel);
            owner.applyComponentStyling(owner.noiseLevelValue);
            owner.applyComponentStyling(owner.noisePanValue);

            // Set value label styling for noise generator (orange accent color)
            owner.noiseLevelValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.noiseLevelValue.setJustificationType(juce::Justification::centred);
            owner.noiseLevelValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.noiseLevelValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.noisePanValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.noisePanValue.setJustificationType(juce::Justification::centred);
            owner.noisePanValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.noisePanValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // Noise combo box options are set up in setupComboBoxOptions() - no need to duplicate here

            // Set labels
            owner.noiseTypeLabel.setText("Type:", juce::dontSendNotification);
            owner.noiseLevelLabel.setText("Level:", juce::dontSendNotification);
            owner.noisePanLabel.setText("Pan:", juce::dontSendNotification);

            // Add ALL components directly to the tab (not to a group) - INCLUDING VALUE LABELS
            addAndMakeVisible(owner.noiseTypeLabel);
            addAndMakeVisible(owner.noiseTypeCombo);
            addAndMakeVisible(owner.noiseLevelLabel);
            addAndMakeVisible(owner.noiseLevelSlider);
            addAndMakeVisible(owner.noiseLevelValue);  // Value label for level
            addAndMakeVisible(owner.noisePanLabel);
            addAndMakeVisible(owner.noisePanSlider);
            addAndMakeVisible(owner.noisePanValue);    // Value label for pan
        }

        void layoutDirectOscillator(FreOscEditor::OscillatorSection& section, juce::Rectangle<int> area, const juce::String& title)
        {
            // DIRECT LAYOUT: Position components directly in the given area
            // This is what we proved works with our test components

            // Store the area for border drawing (add some padding for the border)
            if (title.contains("1")) osc1BorderArea = area;
            else if (title.contains("2")) osc2BorderArea = area;
            else if (title.contains("3")) osc3BorderArea = area;

            // Create a title area (like GroupComponent border) - leave space at top for title
            auto titleArea = area.removeFromTop(20);

            // Top row: Waveform and Octave dropdowns side by side
            auto topRow = area.removeFromTop(50).reduced(5);
            auto waveformArea = topRow.removeFromLeft(topRow.getWidth() / 2).reduced(2);
            auto octaveArea = topRow.reduced(2);

            // Waveform
            section.waveformLabel.setBounds(waveformArea.removeFromTop(15));
            section.waveformCombo.setBounds(waveformArea);

            // Octave
            section.octaveLabel.setBounds(octaveArea.removeFromTop(15));
            section.octaveCombo.setBounds(octaveArea);

            // Bottom row: Level, Detune, Pan knobs side by side with minimum size constraints
            auto bottomRow = area.reduced(5);

            // Calculate minimum space needed for each control
            const int minKnobSize = 50;  // Reduced from 60 to allow more room
            const int minLabelHeight = 15;
            const int minValueHeight = 16;
            const int minTotalWidth = minKnobSize + 10; // knob + padding
            const int minTotalHeight = minLabelHeight + minKnobSize + minValueHeight + 6; // label + knob + value + padding

            // Check if we have enough space, if not, force scrolling by using minimum sizes
            auto availableWidth = bottomRow.getWidth();
            auto availableHeight = bottomRow.getHeight();
            auto knobWidth = availableWidth / 3;

            // If individual knob area would be too small, use minimum size and let scrolling handle it
            if (knobWidth < minTotalWidth || availableHeight < minTotalHeight)
            {
                knobWidth = minTotalWidth;
            }

            // Level
            auto levelArea = juce::Rectangle<int>(bottomRow.getX(), bottomRow.getY(), knobWidth, bottomRow.getHeight()).reduced(3);
            section.levelLabel.setBounds(levelArea.removeFromTop(minLabelHeight));

            // Ensure value area gets minimum height, even if it means making the knob smaller
            auto levelValueArea = levelArea.removeFromBottom(minValueHeight);
            auto levelKnobArea = levelArea;

            // Make sure knob area has at least some height
            if (levelKnobArea.getHeight() < minKnobSize)
            {
                // If not enough space, shrink value area and give knob minimum space
                levelValueArea = levelArea.removeFromBottom(juce::jmax(12, levelArea.getHeight() - minKnobSize));
                levelKnobArea = levelArea;
            }

            auto levelKnobBounds = levelKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 6, levelKnobArea.getWidth()),
                juce::jlimit(minKnobSize, levelKnobArea.getHeight(), levelKnobArea.getHeight())
            );
            section.levelSlider.setBounds(levelKnobBounds);
            section.levelValue.setBounds(levelValueArea);

            // Detune
            auto detuneArea = juce::Rectangle<int>(bottomRow.getX() + knobWidth, bottomRow.getY(), knobWidth, bottomRow.getHeight()).reduced(3);
            section.detuneLabel.setBounds(detuneArea.removeFromTop(minLabelHeight));

            // Ensure value area gets minimum height
            auto detuneValueArea = detuneArea.removeFromBottom(minValueHeight);
            auto detuneKnobArea = detuneArea;

            if (detuneKnobArea.getHeight() < minKnobSize)
            {
                detuneValueArea = detuneArea.removeFromBottom(juce::jmax(12, detuneArea.getHeight() - minKnobSize));
                detuneKnobArea = detuneArea;
            }

            auto detuneKnobBounds = detuneKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 6, detuneKnobArea.getWidth()),
                juce::jlimit(minKnobSize, detuneKnobArea.getHeight(), detuneKnobArea.getHeight())
            );
            section.detuneSlider.setBounds(detuneKnobBounds);
            section.detuneValue.setBounds(detuneValueArea);

            // Pan
            auto panArea = juce::Rectangle<int>(bottomRow.getX() + 2 * knobWidth, bottomRow.getY(), knobWidth, bottomRow.getHeight()).reduced(3);
            section.panLabel.setBounds(panArea.removeFromTop(minLabelHeight));

            // Ensure value area gets minimum height
            auto panValueArea = panArea.removeFromBottom(minValueHeight);
            auto panKnobArea = panArea;

            if (panKnobArea.getHeight() < minKnobSize)
            {
                panValueArea = panArea.removeFromBottom(juce::jmax(12, panArea.getHeight() - minKnobSize));
                panKnobArea = panArea;
            }

            auto panKnobBounds = panKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 6, panKnobArea.getWidth()),
                juce::jlimit(minKnobSize, panKnobArea.getHeight(), panKnobArea.getHeight())
            );
            section.panSlider.setBounds(panKnobBounds);
            section.panValue.setBounds(panValueArea);
        }

        void layoutDirectNoise(juce::Rectangle<int> area)
        {
            // DIRECT LAYOUT: Position noise components directly in the given area

            // Store area for border drawing
            noiseBorderArea = area;

            // Leave space at top for title
            area.removeFromTop(20);

            // Layout: Type dropdown on top, then Level and Pan knobs side by side
            auto topRow = area.removeFromTop(50).reduced(5);

            // Type dropdown (full width)
            owner.noiseTypeLabel.setBounds(topRow.removeFromTop(15));
            owner.noiseTypeCombo.setBounds(topRow);

            // Bottom row: Level and Pan knobs side by side
            auto bottomRow = area.reduced(5);

            // Calculate minimum space needed for each control
            const int minKnobSize = 50;
            const int minLabelHeight = 15;
            const int minValueHeight = 16;
            const int minTotalWidth = minKnobSize + 10; // knob + padding
            const int minTotalHeight = minLabelHeight + minKnobSize + minValueHeight + 6;

            auto availableWidth = bottomRow.getWidth();
            auto availableHeight = bottomRow.getHeight();
            auto knobWidth = availableWidth / 2;

            // If individual knob area would be too small, use minimum size and let scrolling handle it
            if (knobWidth < minTotalWidth || availableHeight < minTotalHeight)
            {
                knobWidth = minTotalWidth;
            }

            // Level
            auto levelArea = juce::Rectangle<int>(bottomRow.getX(), bottomRow.getY(), knobWidth, bottomRow.getHeight()).reduced(3);
            owner.noiseLevelLabel.setBounds(levelArea.removeFromTop(minLabelHeight));

            // Ensure value area gets minimum height
            auto levelValueArea = levelArea.removeFromBottom(minValueHeight);
            auto levelKnobArea = levelArea;

            if (levelKnobArea.getHeight() < minKnobSize)
            {
                levelValueArea = levelArea.removeFromBottom(juce::jmax(12, levelArea.getHeight() - minKnobSize));
                levelKnobArea = levelArea;
            }

            auto levelKnobBounds = levelKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 6, levelKnobArea.getWidth()),
                juce::jlimit(minKnobSize, levelKnobArea.getHeight(), levelKnobArea.getHeight())
            );
            owner.noiseLevelSlider.setBounds(levelKnobBounds);
            owner.noiseLevelValue.setBounds(levelValueArea);

            // Pan
            auto panArea = juce::Rectangle<int>(bottomRow.getX() + knobWidth, bottomRow.getY(), knobWidth, bottomRow.getHeight()).reduced(3);
            owner.noisePanLabel.setBounds(panArea.removeFromTop(minLabelHeight));

            // Ensure value area gets minimum height
            auto panValueArea = panArea.removeFromBottom(minValueHeight);
            auto panKnobArea = panArea;

            if (panKnobArea.getHeight() < minKnobSize)
            {
                panValueArea = panArea.removeFromBottom(juce::jmax(12, panArea.getHeight() - minKnobSize));
                panKnobArea = panArea;
            }

            auto panKnobBounds = panKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 6, panKnobArea.getWidth()),
                juce::jlimit(minKnobSize, panKnobArea.getHeight(), panKnobArea.getHeight())
            );
            owner.noisePanSlider.setBounds(panKnobBounds);
            owner.noisePanValue.setBounds(panValueArea);
        }

        void setupCompleteOscillator(FreOscEditor::OscillatorSection& section, const juce::String& title, const juce::String& paramPrefix)
        {
            // Debug: Starting setup
            DBG("=== Setting up " << title << " ===");

            // Setup group styling
            owner.applyComponentStyling(section.group);
            section.group.setText(title);

            // Setup each component with full configuration
            owner.setupSliderForTab(section.levelSlider, section.levelLabel, section.levelValue, "Level:", paramPrefix + "level");
            owner.setupSliderForTab(section.detuneSlider, section.detuneLabel, section.detuneValue, "Detune:", paramPrefix + "detune");
            owner.setupSliderForTab(section.panSlider, section.panLabel, section.panValue, "Pan:", paramPrefix + "pan");
            owner.setupComboBoxForTab(section.waveformCombo, section.waveformLabel, "Waveform:", paramPrefix + "waveform");
            owner.setupComboBoxForTab(section.octaveCombo, section.octaveLabel, "Octave:", paramPrefix + "octave");

            // Combo box options are set up in setupComboBoxOptions() - no need for additional setup

            // Debug: Check combo box setup
            DBG("Waveform combo items: " << section.waveformCombo.getNumItems());
            DBG("Octave combo items: " << section.octaveCombo.getNumItems());

            // CRITICAL FIX: Ensure components are properly added to their parent group
            // First make sure the group is ready to accept children
            section.group.setSize(400, 200);  // Give it a minimum size

            // Add all components to this oscillator's group (establishing parent-child relationship)
            section.group.addAndMakeVisible(section.waveformCombo);
            section.group.addAndMakeVisible(section.waveformLabel);
            section.group.addAndMakeVisible(section.octaveCombo);
            section.group.addAndMakeVisible(section.octaveLabel);
            section.group.addAndMakeVisible(section.levelSlider);
            section.group.addAndMakeVisible(section.levelLabel);
            section.group.addAndMakeVisible(section.levelValue);
            section.group.addAndMakeVisible(section.detuneSlider);
            section.group.addAndMakeVisible(section.detuneLabel);
            section.group.addAndMakeVisible(section.detuneValue);
            section.group.addAndMakeVisible(section.panSlider);
            section.group.addAndMakeVisible(section.panLabel);
            section.group.addAndMakeVisible(section.panValue);

            // Force immediate layout of child components with basic bounds
            auto groupBounds = juce::Rectangle<int>(0, 20, 400, 180);
            section.waveformLabel.setBounds(10, 30, 80, 20);
            section.waveformCombo.setBounds(10, 50, 120, 25);
            section.octaveLabel.setBounds(140, 30, 80, 20);
            section.octaveCombo.setBounds(140, 50, 80, 25);
            section.levelLabel.setBounds(10, 80, 60, 20);
            section.levelSlider.setBounds(10, 100, 60, 60);
            section.detuneLabel.setBounds(80, 80, 60, 20);
            section.detuneSlider.setBounds(80, 100, 60, 60);
            section.panLabel.setBounds(150, 80, 60, 20);
            section.panSlider.setBounds(150, 100, 60, 60);

            // Explicit visibility calls to ensure components are shown
            section.waveformCombo.setVisible(true);
            section.waveformLabel.setVisible(true);
            section.octaveCombo.setVisible(true);
            section.octaveLabel.setVisible(true);
            section.levelSlider.setVisible(true);
            section.levelLabel.setVisible(true);
            section.levelValue.setVisible(true);
            section.detuneSlider.setVisible(true);
            section.detuneLabel.setVisible(true);
            section.detuneValue.setVisible(true);
            section.panSlider.setVisible(true);
            section.panLabel.setVisible(true);
            section.panValue.setVisible(true);
            section.group.setVisible(true);

            // VISUAL DEBUG: Make osc2 and osc3 components obviously different colors for testing
            if (title.contains("Oscillator 2"))
            {
                section.group.setColour(juce::GroupComponent::outlineColourId, juce::Colours::red);
                section.waveformCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colours::red.withAlpha(0.3f));
                section.octaveCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colours::red.withAlpha(0.3f));
            }
            else if (title.contains("Oscillator 3"))
            {
                section.group.setColour(juce::GroupComponent::outlineColourId, juce::Colours::green);
                section.waveformCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colours::green.withAlpha(0.3f));
                section.octaveCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colours::green.withAlpha(0.3f));
            }
        }

        void setupOscillatorComboBoxOptions([[maybe_unused]] FreOscEditor::OscillatorSection& section)
        {
            // Combo box options are set up in setupComboBoxOptions() - no need to duplicate here
        }

        void setupCompleteNoise()
        {
            // Setup group styling
            owner.applyComponentStyling(owner.noiseGroup);
            owner.noiseGroup.setText("Noise Generator");

            // Setup components
            owner.setupComboBoxForTab(owner.noiseTypeCombo, owner.noiseTypeLabel, "Type:", "noise_type");
            owner.setupSliderForTab(owner.noiseLevelSlider, owner.noiseLevelLabel, owner.noiseLevelValue, "Level:", "noise_level");
            owner.setupSliderForTab(owner.noisePanSlider, owner.noisePanLabel, owner.noisePanValue, "Pan:", "noise_pan");

            // Noise combo box options are set up in setupComboBoxOptions() - no need to duplicate here

            // Add all components to the noise group (establishing parent-child relationship)
            owner.noiseGroup.addAndMakeVisible(owner.noiseTypeCombo);
            owner.noiseGroup.addAndMakeVisible(owner.noiseTypeLabel);
            owner.noiseGroup.addAndMakeVisible(owner.noiseLevelSlider);
            owner.noiseGroup.addAndMakeVisible(owner.noiseLevelLabel);
            owner.noiseGroup.addAndMakeVisible(owner.noiseLevelValue);
            owner.noiseGroup.addAndMakeVisible(owner.noisePanSlider);
            owner.noiseGroup.addAndMakeVisible(owner.noisePanLabel);
            owner.noiseGroup.addAndMakeVisible(owner.noisePanValue);
        }

        void paint(juce::Graphics& g) override
        {
            // Draw borders around oscillator sections
            g.setColour(owner.knobColour);

            // Draw oscillator borders with titles and backgrounds
            if (!osc1BorderArea.isEmpty())
            {
                // Fill background with dark blue
                g.setColour(owner.panelColour.darker(0.2f));
                g.fillRoundedRectangle(osc1BorderArea.toFloat(), 4.0f);
                
                // Draw border
                g.setColour(owner.knobColour);
                g.drawRoundedRectangle(osc1BorderArea.toFloat(), 4.0f, 1.0f);
                
                // Draw title text
                g.setColour(owner.textColour);
                g.setFont(12.0f);
                g.drawText("Oscillator 1", osc1BorderArea.getX() + 10, osc1BorderArea.getY() - 2, 100, 16, juce::Justification::left);
            }

            if (!osc2BorderArea.isEmpty())
            {
                // Fill background with dark blue
                g.setColour(owner.panelColour.darker(0.2f));
                g.fillRoundedRectangle(osc2BorderArea.toFloat(), 4.0f);
                
                // Draw border
                g.setColour(owner.knobColour);
                g.drawRoundedRectangle(osc2BorderArea.toFloat(), 4.0f, 1.0f);
                
                // Draw title text
                g.setColour(owner.textColour);
                g.setFont(12.0f);
                g.drawText("Oscillator 2", osc2BorderArea.getX() + 10, osc2BorderArea.getY() - 2, 100, 16, juce::Justification::left);
            }

            if (!osc3BorderArea.isEmpty())
            {
                // Fill background with dark blue
                g.setColour(owner.panelColour.darker(0.2f));
                g.fillRoundedRectangle(osc3BorderArea.toFloat(), 4.0f);
                
                // Draw border
                g.setColour(owner.knobColour);
                g.drawRoundedRectangle(osc3BorderArea.toFloat(), 4.0f, 1.0f);
                
                // Draw title text
                g.setColour(owner.textColour);
                g.setFont(12.0f);
                g.drawText("Oscillator 3", osc3BorderArea.getX() + 10, osc3BorderArea.getY() - 2, 100, 16, juce::Justification::left);
            }

            if (!noiseBorderArea.isEmpty())
            {
                // Fill background with dark blue
                g.setColour(owner.panelColour.darker(0.2f));
                g.fillRoundedRectangle(noiseBorderArea.toFloat(), 4.0f);
                
                // Draw border
                g.setColour(owner.knobColour);
                g.drawRoundedRectangle(noiseBorderArea.toFloat(), 4.0f, 1.0f);
                
                // Draw title text
                g.setColour(owner.textColour);
                g.setFont(12.0f);
                g.drawText("Noise Generator", noiseBorderArea.getX() + 10, noiseBorderArea.getY() - 2, 120, 16, juce::Justification::left);
            }
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(10);

            auto sectionHeight = bounds.getHeight() / 4;

            // Layout ALL oscillators with direct component positioning
            auto osc1Area = bounds.removeFromTop(sectionHeight).reduced(5);
            layoutDirectOscillator(owner.osc1Section, osc1Area, "Oscillator 1");

            auto osc2Area = bounds.removeFromTop(sectionHeight).reduced(5);
            layoutDirectOscillator(owner.osc2Section, osc2Area, "Oscillator 2");

            auto osc3Area = bounds.removeFromTop(sectionHeight).reduced(5);
            layoutDirectOscillator(owner.osc3Section, osc3Area, "Oscillator 3");

            // Layout noise generator in remaining space
            auto noiseArea = bounds.reduced(5);
            layoutDirectNoise(noiseArea);

            return;

            // Debug: Print bounds information
            DBG("=== Oscillators Tab Resized ===");
            DBG("Total bounds: " << bounds.toString());
            DBG("Section height: " << sectionHeight);

            // Set bounds for each section with debug output
            auto debugOsc1Bounds = bounds.removeFromTop(sectionHeight).reduced(5);
            owner.osc1Section.group.setBounds(debugOsc1Bounds);
            DBG("OSC1 bounds: " << debugOsc1Bounds.toString());

            auto osc2Bounds = bounds.removeFromTop(sectionHeight).reduced(5);
            owner.osc2Section.group.setBounds(osc2Bounds);
            DBG("OSC2 bounds: " << osc2Bounds.toString());

            auto osc3Bounds = bounds.removeFromTop(sectionHeight).reduced(5);
            owner.osc3Section.group.setBounds(osc3Bounds);
            DBG("OSC3 bounds: " << osc3Bounds.toString());

            auto noiseBounds = bounds.reduced(5);
            owner.noiseGroup.setBounds(noiseBounds);
            DBG("Noise bounds: " << noiseBounds.toString());

            // Layout all sections
            owner.layoutOscillatorSection(owner.osc1Section);
            owner.layoutOscillatorSection(owner.osc2Section);
            owner.layoutOscillatorSection(owner.osc3Section);
            owner.layoutNoiseSection();

            // Debug: Check component visibility
            DBG("=== Component Visibility Check ===");
            DBG("OSC1 visible: " << owner.osc1Section.group.isVisible());
            DBG("OSC2 visible: " << owner.osc2Section.group.isVisible());
            DBG("OSC3 visible: " << owner.osc3Section.group.isVisible());
            DBG("OSC2 waveform combo visible: " << owner.osc2Section.waveformCombo.isVisible());
            DBG("OSC3 waveform combo visible: " << owner.osc3Section.waveformCombo.isVisible());
        }

    private:
        FreOscEditor& owner;

        // Test components to verify tab system works
        juce::Label testLabel;
        juce::ComboBox testCombo;

        // Direct test components for osc2 area
        juce::Label directOsc2Label;
        juce::ComboBox directOsc2Combo;

        // Track which sections we're using direct layout for
        bool osc2SectionBounds = false;
        bool osc3SectionBounds = false;

        // Border areas for drawing oscillator borders
        juce::Rectangle<int> osc1BorderArea, osc2BorderArea, osc3BorderArea, noiseBorderArea;
    };

    return std::make_unique<OscillatorsTabComponent>(*this);
}

std::unique_ptr<juce::Component> FreOscEditor::createFilterEnvelopeTab()
{
    class FilterEnvelopeTabComponent : public juce::Component
    {
    public:
        FilterEnvelopeTabComponent(FreOscEditor& editor) : owner(editor)
        {
            // DIRECT LAYOUT: Setup filter and envelope components without GroupComponent
            setupDirectFilter();
            setupDirectEnvelope();
        }

        void setupDirectFilter()
        {
            // DIRECT SETUP: Add filter components directly to tab (no GroupComponent)

            // Setup components with proper styling
            owner.applyComponentStyling(owner.filterTypeCombo);
            owner.applyComponentStyling(owner.cutoffSlider);
            owner.applyComponentStyling(owner.resonanceSlider);
            owner.applyComponentStyling(owner.filterGainSlider);
            owner.applyComponentStyling(owner.formantVowelCombo);

            owner.applyComponentStyling(owner.filterTypeLabel);
            owner.applyComponentStyling(owner.cutoffLabel);
            owner.applyComponentStyling(owner.resonanceLabel);
            owner.applyComponentStyling(owner.filterGainLabel);
            owner.applyComponentStyling(owner.formantLabel);
            owner.applyComponentStyling(owner.cutoffValue);
            owner.applyComponentStyling(owner.resonanceValue);
            owner.applyComponentStyling(owner.filterGainValue);

            // Set value label styling for filter controls (orange accent color)
            owner.cutoffValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.cutoffValue.setJustificationType(juce::Justification::centred);
            owner.cutoffValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.cutoffValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.resonanceValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.resonanceValue.setJustificationType(juce::Justification::centred);
            owner.resonanceValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.resonanceValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.filterGainValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.filterGainValue.setJustificationType(juce::Justification::centred);
            owner.filterGainValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.filterGainValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // Filter and formant combo box options are set up in setupComboBoxOptions() - no need to duplicate here

            // Set labels
            owner.filterTypeLabel.setText("Type:", juce::dontSendNotification);
            owner.cutoffLabel.setText("Cutoff:", juce::dontSendNotification);
            owner.resonanceLabel.setText("Resonance:", juce::dontSendNotification);
            owner.filterGainLabel.setText("Gain:", juce::dontSendNotification);
            owner.formantLabel.setText("Vowel:", juce::dontSendNotification);

            // Add ALL components directly to the tab (not to a group) - INCLUDING VALUE LABELS
            addAndMakeVisible(owner.filterTypeLabel);
            addAndMakeVisible(owner.filterTypeCombo);
            addAndMakeVisible(owner.cutoffLabel);
            addAndMakeVisible(owner.cutoffSlider);
            addAndMakeVisible(owner.cutoffValue);     // Value label for cutoff
            addAndMakeVisible(owner.resonanceLabel);
            addAndMakeVisible(owner.resonanceSlider);
            addAndMakeVisible(owner.resonanceValue);  // Value label for resonance
            addAndMakeVisible(owner.filterGainLabel);
            addAndMakeVisible(owner.filterGainSlider);
            addAndMakeVisible(owner.filterGainValue); // Value label for gain
            addAndMakeVisible(owner.formantLabel);
            addAndMakeVisible(owner.formantVowelCombo);
        }

        void setupDirectEnvelope()
        {
            // DIRECT SETUP: Add envelope components directly to tab (no GroupComponent)

            // Setup components with proper styling
            owner.applyComponentStyling(owner.attackSlider);
            owner.applyComponentStyling(owner.decaySlider);
            owner.applyComponentStyling(owner.sustainSlider);
            owner.applyComponentStyling(owner.releaseSlider);

            owner.applyComponentStyling(owner.attackLabel);
            owner.applyComponentStyling(owner.decayLabel);
            owner.applyComponentStyling(owner.sustainLabel);
            owner.applyComponentStyling(owner.releaseLabel);
            owner.applyComponentStyling(owner.attackValue);
            owner.applyComponentStyling(owner.decayValue);
            owner.applyComponentStyling(owner.sustainValue);
            owner.applyComponentStyling(owner.releaseValue);

            // Set value label styling for envelope controls (orange accent color)
            owner.attackValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.attackValue.setJustificationType(juce::Justification::centred);
            owner.attackValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.attackValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.decayValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.decayValue.setJustificationType(juce::Justification::centred);
            owner.decayValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.decayValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.sustainValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.sustainValue.setJustificationType(juce::Justification::centred);
            owner.sustainValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.sustainValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.releaseValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.releaseValue.setJustificationType(juce::Justification::centred);
            owner.releaseValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.releaseValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // Set labels
            owner.attackLabel.setText("Attack:", juce::dontSendNotification);
            owner.decayLabel.setText("Decay:", juce::dontSendNotification);
            owner.sustainLabel.setText("Sustain:", juce::dontSendNotification);
            owner.releaseLabel.setText("Release:", juce::dontSendNotification);

            // Add ALL components directly to the tab (not to a group) - INCLUDING VALUE LABELS
            addAndMakeVisible(owner.attackLabel);
            addAndMakeVisible(owner.attackSlider);
            addAndMakeVisible(owner.attackValue);   // Value label for attack
            addAndMakeVisible(owner.decayLabel);
            addAndMakeVisible(owner.decaySlider);
            addAndMakeVisible(owner.decayValue);    // Value label for decay
            addAndMakeVisible(owner.sustainLabel);
            addAndMakeVisible(owner.sustainSlider);
            addAndMakeVisible(owner.sustainValue);  // Value label for sustain
            addAndMakeVisible(owner.releaseLabel);
            addAndMakeVisible(owner.releaseSlider);
            addAndMakeVisible(owner.releaseValue);  // Value label for release
        }

        void layoutDirectFilter(juce::Rectangle<int> area)
        {
            // Store area for border drawing
            filterBorderArea = area;

            // Leave space at top for title
            area.removeFromTop(20);

            // Layout: Type and Vowel dropdowns on top, then sliders
            auto topRow = area.removeFromTop(50).reduced(5);

            // Type dropdown (left half)
            auto typeArea = topRow.removeFromLeft(topRow.getWidth() / 2).reduced(2);
            owner.filterTypeLabel.setBounds(typeArea.removeFromTop(15));
            owner.filterTypeCombo.setBounds(typeArea);

            // Vowel dropdown (right half)
            auto vowelArea = topRow.reduced(2);
            owner.formantLabel.setBounds(vowelArea.removeFromTop(15));
            owner.formantVowelCombo.setBounds(vowelArea);

            // Bottom area: Cutoff, Resonance, Gain knobs
            auto bottomRow = area.reduced(5);
            auto knobWidth = bottomRow.getWidth() / 3;

            // Minimum knob size
            const int minKnobSize = 60;
            const int minLabelHeight = 15;

            // Cutoff
            auto cutoffArea = bottomRow.removeFromLeft(knobWidth).reduced(3);
            owner.cutoffLabel.setBounds(cutoffArea.removeFromTop(minLabelHeight));

            // Ensure value area gets minimum height
            auto cutoffValueArea = cutoffArea.removeFromBottom(minLabelHeight);
            auto cutoffKnobArea = cutoffArea;

            if (cutoffKnobArea.getHeight() < minKnobSize)
            {
                cutoffValueArea = cutoffArea.removeFromBottom(juce::jmax(12, cutoffArea.getHeight() - minKnobSize));
                cutoffKnobArea = cutoffArea;
            }

            auto cutoffKnobBounds = cutoffKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, cutoffKnobArea.getWidth()),
                juce::jmax(minKnobSize, cutoffKnobArea.getHeight())
            );
            owner.cutoffSlider.setBounds(cutoffKnobBounds);
            owner.cutoffValue.setBounds(cutoffValueArea);

            // Resonance
            auto resonanceArea = bottomRow.removeFromLeft(knobWidth).reduced(3);
            owner.resonanceLabel.setBounds(resonanceArea.removeFromTop(minLabelHeight));

            // Ensure value area gets minimum height
            auto resonanceValueArea = resonanceArea.removeFromBottom(minLabelHeight);
            auto resonanceKnobArea = resonanceArea;

            if (resonanceKnobArea.getHeight() < minKnobSize)
            {
                resonanceValueArea = resonanceArea.removeFromBottom(juce::jmax(12, resonanceArea.getHeight() - minKnobSize));
                resonanceKnobArea = resonanceArea;
            }

            auto resonanceKnobBounds = resonanceKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, resonanceKnobArea.getWidth()),
                juce::jmax(minKnobSize, resonanceKnobArea.getHeight())
            );
            owner.resonanceSlider.setBounds(resonanceKnobBounds);
            owner.resonanceValue.setBounds(resonanceValueArea);

            // Gain
            auto gainArea = bottomRow.reduced(3);
            owner.filterGainLabel.setBounds(gainArea.removeFromTop(minLabelHeight));

            // Ensure value area gets minimum height
            auto gainValueArea = gainArea.removeFromBottom(minLabelHeight);
            auto gainKnobArea = gainArea;

            if (gainKnobArea.getHeight() < minKnobSize)
            {
                gainValueArea = gainArea.removeFromBottom(juce::jmax(12, gainArea.getHeight() - minKnobSize));
                gainKnobArea = gainArea;
            }

            auto gainKnobBounds = gainKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, gainKnobArea.getWidth()),
                juce::jmax(minKnobSize, gainKnobArea.getHeight())
            );
            owner.filterGainSlider.setBounds(gainKnobBounds);
            owner.filterGainValue.setBounds(gainValueArea);
        }

        void layoutDirectEnvelope(juce::Rectangle<int> area)
        {
            // Store area for border drawing
            envelopeBorderArea = area;

            // Leave space at top for title
            area.removeFromTop(20);

            // Layout ADSR in 2x2 grid
            auto rowHeight = area.getHeight() / 2;
            auto colWidth = area.getWidth() / 2;

            // Minimum sizes
            const int minKnobSize = 60;
            const int minLabelHeight = 15;

            // Attack (top-left)
            auto attackArea = juce::Rectangle<int>(area.getX(), area.getY(), colWidth, rowHeight).reduced(5);
            owner.attackLabel.setBounds(attackArea.removeFromTop(minLabelHeight));

            // Ensure value area gets minimum height
            auto attackValueArea = attackArea.removeFromBottom(minLabelHeight);
            auto attackKnobArea = attackArea;

            if (attackKnobArea.getHeight() < minKnobSize)
            {
                attackValueArea = attackArea.removeFromBottom(juce::jmax(12, attackArea.getHeight() - minKnobSize));
                attackKnobArea = attackArea;
            }

            auto attackKnobBounds = attackKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, attackKnobArea.getWidth()),
                juce::jmax(minKnobSize, attackKnobArea.getHeight())
            );
            owner.attackSlider.setBounds(attackKnobBounds);
            owner.attackValue.setBounds(attackValueArea);

            // Decay (top-right)
            auto decayArea = juce::Rectangle<int>(area.getX() + colWidth, area.getY(), colWidth, rowHeight).reduced(5);
            owner.decayLabel.setBounds(decayArea.removeFromTop(minLabelHeight));

            auto decayValueArea = decayArea.removeFromBottom(minLabelHeight);
            auto decayKnobArea = decayArea;

            if (decayKnobArea.getHeight() < minKnobSize)
            {
                decayValueArea = decayArea.removeFromBottom(juce::jmax(12, decayArea.getHeight() - minKnobSize));
                decayKnobArea = decayArea;
            }

            auto decayKnobBounds = decayKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, decayKnobArea.getWidth()),
                juce::jmax(minKnobSize, decayKnobArea.getHeight())
            );
            owner.decaySlider.setBounds(decayKnobBounds);
            owner.decayValue.setBounds(decayValueArea);

            // Sustain (bottom-left)
            auto sustainArea = juce::Rectangle<int>(area.getX(), area.getY() + rowHeight, colWidth, rowHeight).reduced(5);
            owner.sustainLabel.setBounds(sustainArea.removeFromTop(minLabelHeight));

            auto sustainValueArea = sustainArea.removeFromBottom(minLabelHeight);
            auto sustainKnobArea = sustainArea;

            if (sustainKnobArea.getHeight() < minKnobSize)
            {
                sustainValueArea = sustainArea.removeFromBottom(juce::jmax(12, sustainArea.getHeight() - minKnobSize));
                sustainKnobArea = sustainArea;
            }

            auto sustainKnobBounds = sustainKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, sustainKnobArea.getWidth()),
                juce::jmax(minKnobSize, sustainKnobArea.getHeight())
            );
            owner.sustainSlider.setBounds(sustainKnobBounds);
            owner.sustainValue.setBounds(sustainValueArea);

            // Release (bottom-right)
            auto releaseArea = juce::Rectangle<int>(area.getX() + colWidth, area.getY() + rowHeight, colWidth, rowHeight).reduced(5);
            owner.releaseLabel.setBounds(releaseArea.removeFromTop(minLabelHeight));

            auto releaseValueArea = releaseArea.removeFromBottom(minLabelHeight);
            auto releaseKnobArea = releaseArea;

            if (releaseKnobArea.getHeight() < minKnobSize)
            {
                releaseValueArea = releaseArea.removeFromBottom(juce::jmax(12, releaseArea.getHeight() - minKnobSize));
                releaseKnobArea = releaseArea;
            }

            auto releaseKnobBounds = releaseKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, releaseKnobArea.getWidth()),
                juce::jmax(minKnobSize, releaseKnobArea.getHeight())
            );
            owner.releaseSlider.setBounds(releaseKnobBounds);
            owner.releaseValue.setBounds(releaseValueArea);
        }

        void paint(juce::Graphics& g) override
        {
            // Draw borders around filter and envelope sections
            g.setColour(owner.knobColour);

            // Draw filter border with title and background
            if (!filterBorderArea.isEmpty())
            {
                // Fill background with dark blue
                g.setColour(owner.panelColour.darker(0.2f));
                g.fillRoundedRectangle(filterBorderArea.toFloat(), 4.0f);
                
                // Draw border
                g.setColour(owner.knobColour);
                g.drawRoundedRectangle(filterBorderArea.toFloat(), 4.0f, 1.0f);
                
                // Draw title text
                g.setColour(owner.textColour);
                g.setFont(12.0f);
                g.drawText("Filter", filterBorderArea.getX() + 10, filterBorderArea.getY() - 2, 100, 16, juce::Justification::left);
            }

            // Draw envelope border with title and background
            if (!envelopeBorderArea.isEmpty())
            {
                // Fill background with dark blue
                g.setColour(owner.panelColour.darker(0.2f));
                g.fillRoundedRectangle(envelopeBorderArea.toFloat(), 4.0f);
                
                // Draw border
                g.setColour(owner.knobColour);
                g.drawRoundedRectangle(envelopeBorderArea.toFloat(), 4.0f, 1.0f);
                
                // Draw title text
                g.setColour(owner.textColour);
                g.setFont(12.0f);
                g.drawText("Envelope", envelopeBorderArea.getX() + 10, envelopeBorderArea.getY() - 2, 100, 16, juce::Justification::left);
            }
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(10);

            // Split into two sections side by side
            auto halfWidth = bounds.getWidth() / 2;
            auto filterArea = bounds.removeFromLeft(halfWidth).reduced(5);
            auto envelopeArea = bounds.reduced(5);  // Remaining right half

            // Layout components using direct positioning
            layoutDirectFilter(filterArea);
            layoutDirectEnvelope(envelopeArea);
        }

    private:
        FreOscEditor& owner;

        // Border areas for drawing section borders
        juce::Rectangle<int> filterBorderArea, envelopeBorderArea;
    };

    return std::make_unique<FilterEnvelopeTabComponent>(*this);
}

std::unique_ptr<juce::Component> FreOscEditor::createModulationTab()
{
    class ModulationTabComponent : public juce::Component
    {
    public:
        ModulationTabComponent(FreOscEditor& editor) : owner(editor)
        {
            // DIRECT LAYOUT: Setup LFO and FM components without GroupComponent
            setupDirectLFO();
            setupDirectFM();
        }

        void setupDirectLFO()
        {
            // DIRECT SETUP: Add LFO components directly to tab (no GroupComponent)

            // Setup components with proper styling
            owner.applyComponentStyling(owner.lfoWaveformCombo);
            owner.applyComponentStyling(owner.lfoTargetCombo);
            owner.applyComponentStyling(owner.lfoRateSlider);
            owner.applyComponentStyling(owner.lfoAmountSlider);

            owner.applyComponentStyling(owner.lfoWaveformLabel);
            owner.applyComponentStyling(owner.lfoTargetLabel);
            owner.applyComponentStyling(owner.lfoRateLabel);
            owner.applyComponentStyling(owner.lfoAmountLabel);
            owner.applyComponentStyling(owner.lfoRateValue);
            owner.applyComponentStyling(owner.lfoAmountValue);

            // Set value label styling for LFO controls (orange accent color)
            owner.lfoRateValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.lfoRateValue.setJustificationType(juce::Justification::centred);
            owner.lfoRateValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.lfoRateValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.lfoAmountValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.lfoAmountValue.setJustificationType(juce::Justification::centred);
            owner.lfoAmountValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.lfoAmountValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // LFO combo box options are set up in setupComboBoxOptions() - no need to duplicate here

            // Set labels
            owner.lfoWaveformLabel.setText("Waveform:", juce::dontSendNotification);
            owner.lfoTargetLabel.setText("Target:", juce::dontSendNotification);
            owner.lfoRateLabel.setText("Rate:", juce::dontSendNotification);
            owner.lfoAmountLabel.setText("Amount:", juce::dontSendNotification);

            // Add ALL components directly to the tab (not to a group) - INCLUDING VALUE LABELS
            addAndMakeVisible(owner.lfoWaveformLabel);
            addAndMakeVisible(owner.lfoWaveformCombo);
            addAndMakeVisible(owner.lfoTargetLabel);
            addAndMakeVisible(owner.lfoTargetCombo);
            addAndMakeVisible(owner.lfoRateLabel);
            addAndMakeVisible(owner.lfoRateSlider);
            addAndMakeVisible(owner.lfoRateValue);    // Value label for rate
            addAndMakeVisible(owner.lfoAmountLabel);
            addAndMakeVisible(owner.lfoAmountSlider);
            addAndMakeVisible(owner.lfoAmountValue);  // Value label for amount
        }

        void setupDirectFM()
        {
            // DIRECT SETUP: Add FM components directly to tab (no GroupComponent)

            // Setup components with proper styling
            owner.applyComponentStyling(owner.fmTargetCombo);
            owner.applyComponentStyling(owner.fmAmountSlider);
            owner.applyComponentStyling(owner.fmRatioSlider);

            owner.applyComponentStyling(owner.fmTargetLabel);
            owner.applyComponentStyling(owner.fmAmountLabel);
            owner.applyComponentStyling(owner.fmRatioLabel);
            owner.applyComponentStyling(owner.fmAmountValue);
            owner.applyComponentStyling(owner.fmRatioValue);

            // Set value label styling for FM controls (orange accent color)
            owner.fmAmountValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.fmAmountValue.setJustificationType(juce::Justification::centred);
            owner.fmAmountValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.fmAmountValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.fmRatioValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.fmRatioValue.setJustificationType(juce::Justification::centred);
            owner.fmRatioValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.fmRatioValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // FM combo box options are set up in setupComboBoxOptions() - no need to duplicate here

            // Set labels
            owner.fmTargetLabel.setText("Target:", juce::dontSendNotification);
            owner.fmAmountLabel.setText("Amount:", juce::dontSendNotification);
            owner.fmRatioLabel.setText("Ratio:", juce::dontSendNotification);

            // Add ALL components directly to the tab (not to a group) - INCLUDING VALUE LABELS
            addAndMakeVisible(owner.fmTargetLabel);
            addAndMakeVisible(owner.fmTargetCombo);
            addAndMakeVisible(owner.fmAmountLabel);
            addAndMakeVisible(owner.fmAmountSlider);
            addAndMakeVisible(owner.fmAmountValue);  // Value label for amount
            addAndMakeVisible(owner.fmRatioLabel);
            addAndMakeVisible(owner.fmRatioSlider);
            addAndMakeVisible(owner.fmRatioValue);   // Value label for ratio
        }

        void layoutDirectLFO(juce::Rectangle<int> area)
        {
            // Store area for border drawing
            lfoBorderArea = area;

            // Leave space at top for title
            area.removeFromTop(20);

            // Layout: Waveform and Target dropdowns on top, then Rate and Amount knobs
            auto topRow = area.removeFromTop(50).reduced(5);

            // Waveform dropdown (left half)
            auto waveformArea = topRow.removeFromLeft(topRow.getWidth() / 2).reduced(2);
            owner.lfoWaveformLabel.setBounds(waveformArea.removeFromTop(15));
            owner.lfoWaveformCombo.setBounds(waveformArea);

            // Target dropdown (right half)
            auto targetArea = topRow.reduced(2);
            owner.lfoTargetLabel.setBounds(targetArea.removeFromTop(15));
            owner.lfoTargetCombo.setBounds(targetArea);

            // Bottom area: Rate and Amount knobs side by side
            auto bottomRow = area.reduced(5);
            auto knobWidth = bottomRow.getWidth() / 2;

            // Minimum knob size
            const int minKnobSize = 60;
            const int minLabelHeight = 15;

            // Rate
            auto rateArea = bottomRow.removeFromLeft(knobWidth).reduced(3);
            owner.lfoRateLabel.setBounds(rateArea.removeFromTop(minLabelHeight));

            auto rateValueArea = rateArea.removeFromBottom(minLabelHeight);
            auto rateKnobArea = rateArea;

            if (rateKnobArea.getHeight() < minKnobSize)
            {
                rateValueArea = rateArea.removeFromBottom(juce::jmax(12, rateArea.getHeight() - minKnobSize));
                rateKnobArea = rateArea;
            }

            auto rateKnobBounds = rateKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, rateKnobArea.getWidth()),
                juce::jmax(minKnobSize, rateKnobArea.getHeight())
            );
            owner.lfoRateSlider.setBounds(rateKnobBounds);
            owner.lfoRateValue.setBounds(rateValueArea);

            // Amount
            auto amountArea = bottomRow.reduced(3);
            owner.lfoAmountLabel.setBounds(amountArea.removeFromTop(minLabelHeight));

            auto amountValueArea = amountArea.removeFromBottom(minLabelHeight);
            auto amountKnobArea = amountArea;

            if (amountKnobArea.getHeight() < minKnobSize)
            {
                amountValueArea = amountArea.removeFromBottom(juce::jmax(12, amountArea.getHeight() - minKnobSize));
                amountKnobArea = amountArea;
            }

            auto amountKnobBounds = amountKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, amountKnobArea.getWidth()),
                juce::jmax(minKnobSize, amountKnobArea.getHeight())
            );
            owner.lfoAmountSlider.setBounds(amountKnobBounds);
            owner.lfoAmountValue.setBounds(amountValueArea);
        }

        void layoutDirectFM(juce::Rectangle<int> area)
        {
            // Store area for border drawing
            fmBorderArea = area;

            // Leave space at top for title
            area.removeFromTop(20);

            // Layout: Target dropdown on top (full width), then Amount and Ratio knobs
            auto topRow = area.removeFromTop(50).reduced(5);

            // Target dropdown (full width) - Source is always Osc3
            owner.fmTargetLabel.setBounds(topRow.removeFromTop(15));
            owner.fmTargetCombo.setBounds(topRow);

            // Bottom area: Amount and Ratio knobs side by side
            auto bottomRow = area.reduced(5);
            auto knobWidth = bottomRow.getWidth() / 2;

            // Minimum knob size
            const int minKnobSize = 60;
            const int minLabelHeight = 15;

            // Amount
            auto amountArea = bottomRow.removeFromLeft(knobWidth).reduced(3);
            owner.fmAmountLabel.setBounds(amountArea.removeFromTop(minLabelHeight));

            auto amountValueArea = amountArea.removeFromBottom(minLabelHeight);
            auto amountKnobArea = amountArea;

            if (amountKnobArea.getHeight() < minKnobSize)
            {
                amountValueArea = amountArea.removeFromBottom(juce::jmax(12, amountArea.getHeight() - minKnobSize));
                amountKnobArea = amountArea;
            }

            auto amountKnobBounds = amountKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, amountKnobArea.getWidth()),
                juce::jmax(minKnobSize, amountKnobArea.getHeight())
            );
            owner.fmAmountSlider.setBounds(amountKnobBounds);
            owner.fmAmountValue.setBounds(amountValueArea);

            // Ratio
            auto ratioArea = bottomRow.reduced(3);
            owner.fmRatioLabel.setBounds(ratioArea.removeFromTop(minLabelHeight));

            auto ratioValueArea = ratioArea.removeFromBottom(minLabelHeight);
            auto ratioKnobArea = ratioArea;

            if (ratioKnobArea.getHeight() < minKnobSize)
            {
                ratioValueArea = ratioArea.removeFromBottom(juce::jmax(12, ratioArea.getHeight() - minKnobSize));
                ratioKnobArea = ratioArea;
            }

            auto ratioKnobBounds = ratioKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, ratioKnobArea.getWidth()),
                juce::jmax(minKnobSize, ratioKnobArea.getHeight())
            );
            owner.fmRatioSlider.setBounds(ratioKnobBounds);
            owner.fmRatioValue.setBounds(ratioValueArea);
        }

        void paint(juce::Graphics& g) override
        {
            // Draw borders around LFO and FM sections
            g.setColour(owner.knobColour);

            // Draw LFO border with title and background
            if (!lfoBorderArea.isEmpty())
            {
                // Fill background with dark blue
                g.setColour(owner.panelColour.darker(0.2f));
                g.fillRoundedRectangle(lfoBorderArea.toFloat(), 4.0f);
                
                // Draw border
                g.setColour(owner.knobColour);
                g.drawRoundedRectangle(lfoBorderArea.toFloat(), 4.0f, 1.0f);
                
                // Draw title text
                g.setColour(owner.textColour);
                g.setFont(12.0f);
                g.drawText("LFO", lfoBorderArea.getX() + 10, lfoBorderArea.getY() - 2, 100, 16, juce::Justification::left);
            }

            // Draw FM border with title and background
            if (!fmBorderArea.isEmpty())
            {
                // Fill background with dark blue
                g.setColour(owner.panelColour.darker(0.2f));
                g.fillRoundedRectangle(fmBorderArea.toFloat(), 4.0f);
                
                // Draw border
                g.setColour(owner.knobColour);
                g.drawRoundedRectangle(fmBorderArea.toFloat(), 4.0f, 1.0f);
                
                // Draw title text
                g.setColour(owner.textColour);
                g.setFont(12.0f);
                g.drawText("FM Synthesis", fmBorderArea.getX() + 10, fmBorderArea.getY() - 2, 100, 16, juce::Justification::left);
            }
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(10);

            // Split into two sections side by side
            auto halfWidth = bounds.getWidth() / 2;
            auto lfoArea = bounds.removeFromLeft(halfWidth).reduced(5);
            auto fmArea = bounds.reduced(5);  // Remaining right half

            // Layout components using direct positioning
            layoutDirectLFO(lfoArea);
            layoutDirectFM(fmArea);
        }

    private:
        FreOscEditor& owner;

        // Border areas for drawing section borders
        juce::Rectangle<int> lfoBorderArea, fmBorderArea;
    };

    return std::make_unique<ModulationTabComponent>(*this);
}

std::unique_ptr<juce::Component> FreOscEditor::createEffectsTab()
{
    class EffectsTabComponent : public juce::Component
    {
    public:
        EffectsTabComponent(FreOscEditor& editor) : owner(editor)
        {
            // DIRECT LAYOUT: Setup reverb and delay components without GroupComponent
            setupDirectReverb();
            setupDirectDelay();
        }


        void setupDirectReverb()
        {
            // DIRECT SETUP: Add reverb components directly to tab (no GroupComponent)

            // Setup components with proper styling
            owner.applyComponentStyling(owner.roomSizeSlider);
            owner.applyComponentStyling(owner.reverbWetSlider);

            owner.applyComponentStyling(owner.roomSizeLabel);
            owner.applyComponentStyling(owner.reverbWetLabel);
            owner.applyComponentStyling(owner.roomSizeValue);
            owner.applyComponentStyling(owner.reverbWetValue);

            // Set value label styling for reverb controls (orange accent color)
            owner.roomSizeValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.roomSizeValue.setJustificationType(juce::Justification::centred);
            owner.roomSizeValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.roomSizeValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.reverbWetValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.reverbWetValue.setJustificationType(juce::Justification::centred);
            owner.reverbWetValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.reverbWetValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // Set labels
            owner.roomSizeLabel.setText("Room Size:", juce::dontSendNotification);
            owner.reverbWetLabel.setText("Wet Level:", juce::dontSendNotification);

            // Add ALL components directly to the tab (not to a group) - INCLUDING VALUE LABELS
            addAndMakeVisible(owner.roomSizeLabel);
            addAndMakeVisible(owner.roomSizeSlider);
            addAndMakeVisible(owner.roomSizeValue);   // Value label for room size
            addAndMakeVisible(owner.reverbWetLabel);
            addAndMakeVisible(owner.reverbWetSlider);
            addAndMakeVisible(owner.reverbWetValue);  // Value label for wet level
        }

        void setupDirectDelay()
        {
            // DIRECT SETUP: Add delay components directly to tab (no GroupComponent)

            // Setup components with proper styling
            owner.applyComponentStyling(owner.delayTimeSlider);
            owner.applyComponentStyling(owner.delayFeedbackSlider);
            owner.applyComponentStyling(owner.delayWetSlider);

            owner.applyComponentStyling(owner.delayTimeLabel);
            owner.applyComponentStyling(owner.delayFeedbackLabel);
            owner.applyComponentStyling(owner.delayWetLabel);
            owner.applyComponentStyling(owner.delayTimeValue);
            owner.applyComponentStyling(owner.delayFeedbackValue);
            owner.applyComponentStyling(owner.delayWetValue);

            // Set value label styling for delay controls (orange accent color)
            owner.delayTimeValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.delayTimeValue.setJustificationType(juce::Justification::centred);
            owner.delayTimeValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.delayTimeValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.delayFeedbackValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.delayFeedbackValue.setJustificationType(juce::Justification::centred);
            owner.delayFeedbackValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.delayFeedbackValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.delayWetValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.delayWetValue.setJustificationType(juce::Justification::centred);
            owner.delayWetValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.delayWetValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // Set labels
            owner.delayTimeLabel.setText("Time:", juce::dontSendNotification);
            owner.delayFeedbackLabel.setText("Feedback:", juce::dontSendNotification);
            owner.delayWetLabel.setText("Wet Level:", juce::dontSendNotification);

            // Add ALL components directly to the tab (not to a group) - INCLUDING VALUE LABELS
            addAndMakeVisible(owner.delayTimeLabel);
            addAndMakeVisible(owner.delayTimeSlider);
            addAndMakeVisible(owner.delayTimeValue);      // Value label for time
            addAndMakeVisible(owner.delayFeedbackLabel);
            addAndMakeVisible(owner.delayFeedbackSlider);
            addAndMakeVisible(owner.delayFeedbackValue);  // Value label for feedback
            addAndMakeVisible(owner.delayWetLabel);
            addAndMakeVisible(owner.delayWetSlider);
            addAndMakeVisible(owner.delayWetValue);       // Value label for wet level
        }


        void layoutDirectReverb(juce::Rectangle<int> area)
        {
            // Store area for border drawing
            reverbBorderArea = area;

            // Leave space at top for title
            area.removeFromTop(20);

            // Layout Room Size and Wet Level side by side
            auto knobWidth = area.getWidth() / 2;

            // Minimum knob size
            const int minKnobSize = 60;
            const int minLabelHeight = 15;

            // Room Size (left)
            auto roomSizeArea = area.removeFromLeft(knobWidth).reduced(5);
            owner.roomSizeLabel.setBounds(roomSizeArea.removeFromTop(minLabelHeight));
            auto roomSizeKnobArea = roomSizeArea.removeFromTop(roomSizeArea.getHeight() - minLabelHeight);
            auto roomSizeKnobBounds = roomSizeKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, roomSizeKnobArea.getWidth()),
                juce::jmax(minKnobSize, roomSizeKnobArea.getHeight())
            );
            owner.roomSizeSlider.setBounds(roomSizeKnobBounds);
            owner.roomSizeValue.setBounds(roomSizeArea);

            // Wet Level (right)
            auto wetArea = area.reduced(5);
            owner.reverbWetLabel.setBounds(wetArea.removeFromTop(minLabelHeight));
            auto wetKnobArea = wetArea.removeFromTop(wetArea.getHeight() - minLabelHeight);
            auto wetKnobBounds = wetKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, wetKnobArea.getWidth()),
                juce::jmax(minKnobSize, wetKnobArea.getHeight())
            );
            owner.reverbWetSlider.setBounds(wetKnobBounds);
            owner.reverbWetValue.setBounds(wetArea);
        }

        void layoutDirectDelay(juce::Rectangle<int> area)
        {
            // Store area for border drawing
            delayBorderArea = area;

            // Leave space at top for title
            area.removeFromTop(20);

            // Layout Time, Feedback, Wet Level side by side
            auto knobWidth = area.getWidth() / 3;

            // Minimum knob size
            const int minKnobSize = 60;
            const int minLabelHeight = 15;

            // Time (left)
            auto timeArea = area.removeFromLeft(knobWidth).reduced(3);
            owner.delayTimeLabel.setBounds(timeArea.removeFromTop(minLabelHeight));
            auto timeKnobArea = timeArea.removeFromTop(timeArea.getHeight() - minLabelHeight);
            auto timeKnobBounds = timeKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, timeKnobArea.getWidth()),
                juce::jmax(minKnobSize, timeKnobArea.getHeight())
            );
            owner.delayTimeSlider.setBounds(timeKnobBounds);
            owner.delayTimeValue.setBounds(timeArea);

            // Feedback (center)
            auto feedbackArea = area.removeFromLeft(knobWidth).reduced(3);
            owner.delayFeedbackLabel.setBounds(feedbackArea.removeFromTop(minLabelHeight));
            auto feedbackKnobArea = feedbackArea.removeFromTop(feedbackArea.getHeight() - minLabelHeight);
            auto feedbackKnobBounds = feedbackKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, feedbackKnobArea.getWidth()),
                juce::jmax(minKnobSize, feedbackKnobArea.getHeight())
            );
            owner.delayFeedbackSlider.setBounds(feedbackKnobBounds);
            owner.delayFeedbackValue.setBounds(feedbackArea);

            // Wet Level (right)
            auto wetArea = area.reduced(3);
            owner.delayWetLabel.setBounds(wetArea.removeFromTop(minLabelHeight));
            auto wetKnobArea = wetArea.removeFromTop(wetArea.getHeight() - minLabelHeight);
            auto wetKnobBounds = wetKnobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, wetKnobArea.getWidth()),
                juce::jmax(minKnobSize, wetKnobArea.getHeight())
            );
            owner.delayWetSlider.setBounds(wetKnobBounds);
            owner.delayWetValue.setBounds(wetArea);
        }

        void paint(juce::Graphics& g) override
        {
            // Draw borders around reverb and delay sections
            g.setColour(owner.knobColour);

            // Draw reverb border with title and background
            if (!reverbBorderArea.isEmpty())
            {
                // Fill background with dark blue
                g.setColour(owner.panelColour.darker(0.2f));
                g.fillRoundedRectangle(reverbBorderArea.toFloat(), 4.0f);
                
                // Draw border
                g.setColour(owner.knobColour);
                g.drawRoundedRectangle(reverbBorderArea.toFloat(), 4.0f, 1.0f);
                
                // Draw title text
                g.setColour(owner.textColour);
                g.setFont(12.0f);
                g.drawText("Reverb", reverbBorderArea.getX() + 10, reverbBorderArea.getY() - 2, 100, 16, juce::Justification::left);
            }

            // Draw delay border with title and background
            if (!delayBorderArea.isEmpty())
            {
                // Fill background with dark blue
                g.setColour(owner.panelColour.darker(0.2f));
                g.fillRoundedRectangle(delayBorderArea.toFloat(), 4.0f);
                
                // Draw border
                g.setColour(owner.knobColour);
                g.drawRoundedRectangle(delayBorderArea.toFloat(), 4.0f, 1.0f);
                
                // Draw title text
                g.setColour(owner.textColour);
                g.setFont(12.0f);
                g.drawText("Delay", delayBorderArea.getX() + 10, delayBorderArea.getY() - 2, 100, 16, juce::Justification::left);
            }
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(10);

            // Arrange two sections vertically (reverb and delay)
            auto sectionHeight = bounds.getHeight() / 2;

            auto reverbArea = bounds.removeFromTop(sectionHeight).reduced(5);
            auto delayArea = bounds.reduced(5);

            // Layout components using direct positioning
            layoutDirectReverb(reverbArea);
            layoutDirectDelay(delayArea);
        }

    private:
        FreOscEditor& owner;

        // Border areas for drawing section borders
        juce::Rectangle<int> reverbBorderArea, delayBorderArea;
    };

    return std::make_unique<EffectsTabComponent>(*this);
}

std::unique_ptr<juce::Component> FreOscEditor::createMasterTab()
{
    class MasterTabComponent : public juce::Component
    {
    public:
        MasterTabComponent(FreOscEditor& editor) : owner(editor)
        {
            // DIRECT LAYOUT: Setup master components without GroupComponent
            setupDirectMaster();
        }

        void setupDirectMaster()
        {
            // DIRECT SETUP: Add master components directly to tab (no GroupComponent)

            // Setup components with proper styling
            owner.applyComponentStyling(owner.masterVolumeSlider);
            owner.applyComponentStyling(owner.masterVolumeLabel);
            owner.applyComponentStyling(owner.masterVolumeValue);

            // Set value label styling for master volume (orange accent color)
            owner.masterVolumeValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.masterVolumeValue.setJustificationType(juce::Justification::centred);
            owner.masterVolumeValue.setColour(juce::Label::textColourId, owner.accentColour.brighter(0.1f));
            owner.masterVolumeValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // Set label
            owner.masterVolumeLabel.setText("Volume:", juce::dontSendNotification);

            // Add ALL components directly to the tab (not to a group) - INCLUDING VALUE LABELS
            addAndMakeVisible(owner.masterVolumeLabel);
            addAndMakeVisible(owner.masterVolumeSlider);
            addAndMakeVisible(owner.masterVolumeValue);  // Value label for master volume
        }

        void layoutDirectMaster(juce::Rectangle<int> area)
        {
            // Store area for border drawing
            masterBorderArea = area;

            // Leave space at top for title
            area.removeFromTop(20);

            // Center the master volume control
            auto controlArea = area.withSizeKeepingCentre(150, area.getHeight());

            // Minimum knob size
            const int minKnobSize = 80; // Larger knob for master volume
            const int minLabelHeight = 15;

            // Master Volume
            owner.masterVolumeLabel.setBounds(controlArea.removeFromTop(minLabelHeight));
            auto knobArea = controlArea.removeFromTop(controlArea.getHeight() - minLabelHeight);
            auto knobBounds = knobArea.withSizeKeepingCentre(
                juce::jmax(minKnobSize, knobArea.getWidth()),
                juce::jmax(minKnobSize, knobArea.getHeight())
            );
            owner.masterVolumeSlider.setBounds(knobBounds);
            owner.masterVolumeValue.setBounds(controlArea);
        }

        void paint(juce::Graphics& g) override
        {
            // Draw border around master section
            g.setColour(owner.knobColour);

            // Draw master border with title and background
            if (!masterBorderArea.isEmpty())
            {
                // Fill background with dark blue
                g.setColour(owner.panelColour.darker(0.2f));
                g.fillRoundedRectangle(masterBorderArea.toFloat(), 4.0f);
                
                // Draw border
                g.setColour(owner.knobColour);
                g.drawRoundedRectangle(masterBorderArea.toFloat(), 4.0f, 1.0f);
                
                // Draw title text
                g.setColour(owner.textColour);
                g.setFont(12.0f);
                g.drawText("Master", masterBorderArea.getX() + 10, masterBorderArea.getY() - 2, 100, 16, juce::Justification::left);
            }
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(10);

            // Master section in center
            auto masterArea = bounds.withSizeKeepingCentre(300, 200);

            // Layout components using direct positioning
            layoutDirectMaster(masterArea);
        }

    private:
        FreOscEditor& owner;

        // Border area for drawing section border
        juce::Rectangle<int> masterBorderArea;
    };

    return std::make_unique<MasterTabComponent>(*this);
}

//==============================================================================
// Tab-specific setup methods

void FreOscEditor::setupComponentStyling(juce::Component& component)
{
    // Use the existing setupComponent method
    setupComponent(component);
}


void FreOscEditor::setupComboBoxForTab(juce::ComboBox& combo, juce::Label& label,
                                       const juce::String& labelText, const juce::String& parameterID)
{
    // Apply styling without adding to main editor
    applyComponentStyling(combo);
    applyComponentStyling(label);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions().withHeight(12.0f * currentScale)));
    label.setJustificationType(juce::Justification::centred);

    // Parameter ID is available for future parameter setup if needed
    juce::ignoreUnused(parameterID);
}

void FreOscEditor::setupSliderForTab(juce::Slider& slider, juce::Label& label, juce::Label& valueLabel,
                                     const juce::String& labelText, const juce::String& parameterID)
{
    // Apply styling without adding to main editor
    applyComponentStyling(slider);
    applyComponentStyling(label);
    applyComponentStyling(valueLabel);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions().withHeight(12.0f * currentScale)));
    label.setJustificationType(juce::Justification::centred);

    valueLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f * currentScale)));
    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setColour(juce::Label::textColourId, accentColour.brighter(0.1f));
    valueLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    // Set parameter range from processor
    auto* param = audioProcessor.getValueTreeState().getParameter(parameterID);
    if (param != nullptr)
    {
        auto range = audioProcessor.getValueTreeState().getParameterRange(parameterID);
        slider.setRange(range.start, range.end, range.interval);
        slider.setValue(param->getValue() * (range.end - range.start) + range.start, juce::dontSendNotification);
    }
}

void FreOscEditor::setupOscillatorSectionForTab(OscillatorSection& section, const juce::String& title,
                                                const juce::String& paramPrefix)
{
    // Apply styling to group without adding to main editor
    applyComponentStyling(section.group);
    section.group.setText(title);

    // Setup components without adding to main editor
    setupComboBoxForTab(section.waveformCombo, section.waveformLabel, "Waveform:", paramPrefix + "waveform");
    setupComboBoxForTab(section.octaveCombo, section.octaveLabel, "Octave:", paramPrefix + "octave");
    setupSliderForTab(section.levelSlider, section.levelLabel, section.levelValue, "Level:", paramPrefix + "level");
    setupSliderForTab(section.detuneSlider, section.detuneLabel, section.detuneValue, "Detune:", paramPrefix + "detune");
    setupSliderForTab(section.panSlider, section.panLabel, section.panValue, "Pan:", paramPrefix + "pan");
}

void FreOscEditor::applyComponentStyling(juce::Component& component)
{
    // Apply styling without calling addAndMakeVisible
    // This is the same as setupComponent but without adding to main editor

    // Apply FL Studio 3x Osc inspired styling - metallic grey look
    if (auto* label = dynamic_cast<juce::Label*>(&component))
    {
        label->setColour(juce::Label::textColourId, textColour);
        label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label->setJustificationType(juce::Justification::centred);
    }
    else if (auto* slider = dynamic_cast<juce::Slider*>(&component))
    {
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        // Metallic knob styling like FL Studio
        slider->setColour(juce::Slider::rotarySliderFillColourId, accentColour);
        slider->setColour(juce::Slider::rotarySliderOutlineColourId, knobColour.darker(0.3f));
        slider->setColour(juce::Slider::thumbColourId, juce::Colour(0xff87ceeb)); // Light blue thumb
        slider->setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    }
    else if (auto* combo = dynamic_cast<juce::ComboBox*>(&component))
    {
        combo->setColour(juce::ComboBox::backgroundColourId, panelColour.brighter(0.1f));
        combo->setColour(juce::ComboBox::textColourId, textColour);
        combo->setColour(juce::ComboBox::outlineColourId, knobColour);
        combo->setColour(juce::ComboBox::arrowColourId, textColour);
        combo->setColour(juce::ComboBox::buttonColourId, panelColour);
    }
    else if (auto* button = dynamic_cast<juce::TextButton*>(&component))
    {
        button->setColour(juce::TextButton::buttonColourId, panelColour);
        button->setColour(juce::TextButton::buttonOnColourId, accentColour);
        button->setColour(juce::TextButton::textColourOffId, textColour);
        button->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    }
    else if (auto* group = dynamic_cast<juce::GroupComponent*>(&component))
    {
        group->setColour(juce::GroupComponent::outlineColourId, knobColour);
        group->setColour(juce::GroupComponent::textColourId, textColour);
    }
}