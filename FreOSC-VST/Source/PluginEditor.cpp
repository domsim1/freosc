#include "PluginEditor.h"

//==============================================================================
// FreOSC blue theme - elegant blue gradient interface
const juce::Colour FreOscEditor::backgroundColour = juce::Colour(0xff2d4a87);  // Deep blue background
const juce::Colour FreOscEditor::panelColour = juce::Colour(0xff3e5a99);       // Lighter blue panels
const juce::Colour FreOscEditor::accentColour = juce::Colour(0xff5ba3d0);      // Light blue accent
const juce::Colour FreOscEditor::textColour = juce::Colour(0xffc4d6ee);        // Light blue-grey text
const juce::Colour FreOscEditor::knobColour = juce::Colour(0xff1e3a5f);        // Dark blue for controls

// LFO color now handled in LookAndFeel - no static color needed

//==============================================================================
// Custom LookAndFeel to remove tab gradients and style knobs
class FreOscLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver, bool isMouseDown) override
    {
        juce::ignoreUnused(isMouseDown);
        
        const auto area = button.getActiveArea();
        const auto isActive = button.isFrontTab();
        
        // Match waveform selector styling exactly - vintage 80s synth button with 3D depth
        auto buttonBounds = area.toFloat().reduced(1.0f); // Small gap between buttons
        
        if (isActive)
        {
            // PUSHED DOWN button (selected state) - matching waveform selector selected state
            // Dark recessed background
            g.setColour(juce::Colour(0xff1a1a1a)); // Very dark background
            g.fillRoundedRectangle(buttonBounds, 1.0f); // Reduced corner radius to match waveform
            
            // Dark shadow on top and left (inset look)
            g.setColour(juce::Colour(0xff0d0d0d)); // Even darker shadow
            g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                      buttonBounds.getRight(), buttonBounds.getY(), 1.0f); // Top shadow
            g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                      buttonBounds.getX(), buttonBounds.getBottom(), 1.0f); // Left shadow
            
            // Subtle highlight on bottom and right (for depth)
            g.setColour(juce::Colour(0xff404040));
            g.drawLine(buttonBounds.getX(), buttonBounds.getBottom() - 1, 
                      buttonBounds.getRight(), buttonBounds.getBottom() - 1, 1.0f); // Bottom highlight
            g.drawLine(buttonBounds.getRight() - 1, buttonBounds.getY(), 
                      buttonBounds.getRight() - 1, buttonBounds.getBottom(), 1.0f); // Right highlight
        }
        else
        {
            // RAISED button (unselected state) - matching waveform selector unselected state
            // Black plastic background with subtle variation for hover
            auto bgColor = isMouseOver ? 
                          juce::Colour(0xff383838) : juce::Colour(0xff2a2a2a);
            
            g.setColour(bgColor);
            g.fillRoundedRectangle(buttonBounds, 1.0f); // Reduced corner radius to match waveform
            
            // Bright highlight on top and left (raised black plastic look)
            g.setColour(juce::Colour(0xff707070)); // Medium grey highlight for black plastic
            g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                      buttonBounds.getRight(), buttonBounds.getY(), 1.0f); // Top highlight
            g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                      buttonBounds.getX(), buttonBounds.getBottom(), 1.0f); // Left highlight
            
            // Dark shadow on bottom and right (for 3D depth)
            g.setColour(juce::Colour(0xff0a0a0a)); // Very dark shadow for black plastic
            g.drawLine(buttonBounds.getX(), buttonBounds.getBottom() - 1, 
                      buttonBounds.getRight(), buttonBounds.getBottom() - 1, 1.0f); // Bottom shadow
            g.drawLine(buttonBounds.getRight() - 1, buttonBounds.getY(), 
                      buttonBounds.getRight() - 1, buttonBounds.getBottom(), 1.0f); // Right shadow
        }
        
        // Draw text with high contrast colors - matching waveform selector text styling
        // Selected tabs have bright white text, unselected have light grey text
        g.setColour(isActive ? juce::Colours::white : juce::Colour(0xffa0a0a0));
        g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f))); // Reduced font size to match waveform
        
        // Adjust text position based on button state (pressed buttons have offset content)
        auto textArea = buttonBounds.toNearestInt();
        if (isActive)
        {
            // Offset the text down and right when button is pressed - matching waveform behavior
            textArea = textArea.translated(1, 1);
        }
        
        g.drawText(button.getButtonText(), textArea, juce::Justification::centred);
    }
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override
    {
        juce::ignoreUnused(slider, rotaryStartAngle, rotaryEndAngle);
        
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.2f; // Reduced from 2.0f to 2.2f to make knobs smaller
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        
        // Use standard knob rotation: 270 degrees centered at 12 o'clock
        auto startAngle = -juce::MathConstants<float>::pi * 0.5f - juce::MathConstants<float>::pi * 0.75f;  // 7:30 position
        auto endAngle = -juce::MathConstants<float>::pi * 0.5f + juce::MathConstants<float>::pi * 0.75f;    // 4:30 position  
        auto angle = startAngle + sliderPos * (endAngle - startAngle);
        
        // Calculate sizes for different parts
        auto outerRadius = radius * 0.75f;           // Outer ring (reduced from 0.85f)
        auto middleRadius = radius * 0.55f;          // Middle dark ring (reduced from 0.65f)
        auto knobRadius = radius * 0.50f;            // Inner metallic knob
        
        // Draw outer shadow for depth
        auto shadowRadius = outerRadius + 3.0f;
        juce::ColourGradient shadowGradient(
            juce::Colour(0x60000000), centreX + 1.0f, centreY + 1.0f,
            juce::Colour(0x00000000), centreX + shadowRadius, centreY + shadowRadius,
            true);
        g.setGradientFill(shadowGradient);
        g.fillEllipse(centreX - shadowRadius, centreY - shadowRadius, shadowRadius * 2.0f, shadowRadius * 2.0f);
        
        // Draw LED ring indicators around the outside
        const int numLEDs = 15;  // Number of LED positions
        
        // Use smaller LEDs for master volume knob only
        bool isMasterVolume = slider.getName() == "master_volume_knob";
        const float ledRadius = isMasterVolume ? 1.5f : 2.0f; // Smaller LEDs for master volume
        const float ledRingRadius = outerRadius + 6.0f; // Reduced from 8.0f to 6.0f to keep LEDs in bounds
        
        for (int i = 0; i < numLEDs; ++i)
        {
            float ledAngle = startAngle + (i / float(numLEDs - 1)) * (endAngle - startAngle);
            float ledX = centreX + std::cos(ledAngle) * ledRingRadius;
            float ledY = centreY + std::sin(ledAngle) * ledRingRadius;
            
            // Calculate LED position relative to knob (0-1)
            float ledProportion = i / float(numLEDs - 1);
            
            // Calculate if this LED should be lit (traditional level meter behavior)
            bool isLit = ledProportion <= sliderPos;
            
            // Calculate distance from this LED to the current knob position (for gradient effect)
            float distanceFromSlider = std::abs(sliderPos - ledProportion);
            
            // Create gradient effect around knob position
            float gradientRadius = 0.15f; // Radius for gradient effect (about 2 LEDs)
            bool inGradient = distanceFromSlider <= gradientRadius;
            
            // Draw black outer ring for all LEDs
            g.setColour(juce::Colour(0xff000000));  // Black ring
            g.fillEllipse(ledX - ledRadius * 1.1f, ledY - ledRadius * 1.1f, ledRadius * 2.2f, ledRadius * 2.2f);
            
            if (isLit || inGradient)
            {
                float brightness;
                
                if (inGradient)
                {
                    // Within gradient radius - bright gradient effect
                    brightness = juce::jlimit(0.5f, 1.0f, 1.0f - (distanceFromSlider / gradientRadius) * 0.5f);
                }
                else if (isLit)
                {
                    // Normal lit LED outside gradient - good visibility
                    brightness = 0.65f;
                }
                else
                {
                    // Should not reach here, but safety fallback
                    brightness = 0.2f;
                }
                
                // Scale the red color by brightness
                juce::uint8 redValue = (juce::uint8)(255 * brightness);
                juce::uint8 greenValue = (juce::uint8)(34 * brightness); // Keep the orange tint
                juce::Colour ledColour = juce::Colour(redValue, greenValue, (juce::uint8)0, (juce::uint8)255);
                
                g.setColour(ledColour);
                g.fillEllipse(ledX - ledRadius, ledY - ledRadius, ledRadius * 2.0f, ledRadius * 2.0f);
                
                // Add glow effect - stronger for brighter LEDs
                float glowAlpha = brightness * 0.6f;
                g.setColour(juce::Colour(redValue, greenValue, (juce::uint8)0, (juce::uint8)(255 * glowAlpha)));
                g.fillEllipse(ledX - ledRadius * 1.5f, ledY - ledRadius * 1.5f, ledRadius * 3.0f, ledRadius * 3.0f);
            }
            else
            {
                // Unlit LED - dark red color
                g.setColour(juce::Colour(0xff331111));  // Dark red/black
                g.fillEllipse(ledX - ledRadius, ledY - ledRadius, ledRadius * 2.0f, ledRadius * 2.0f);
            }
        }
        
        // Draw outer dark ring (knob body)
        auto outerRingBounds = juce::Rectangle<float>(centreX - outerRadius, centreY - outerRadius, outerRadius * 2.0f, outerRadius * 2.0f);
        juce::ColourGradient outerRingGradient(
            juce::Colour(0xff2a2a2a), centreX - outerRadius * 0.3f, centreY - outerRadius * 0.3f,
            juce::Colour(0xff101010), centreX + outerRadius * 0.3f, centreY + outerRadius * 0.3f,
            false);
        g.setGradientFill(outerRingGradient);
        g.fillEllipse(outerRingBounds);
        
        // Draw middle recessed ring
        auto middleRingBounds = juce::Rectangle<float>(centreX - middleRadius, centreY - middleRadius, middleRadius * 2.0f, middleRadius * 2.0f);
        g.setColour(juce::Colour(0xff1a1a1a));  // Very dark recess
        g.fillEllipse(middleRingBounds);
        
        // Draw metallic center knob with brushed metal effect (matching vertical slider thumbs)
        auto knobBounds = juce::Rectangle<float>(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
        
        // Create realistic metallic gradient (matching the vertical slider thumb style)
        juce::ColourGradient metalGradient(
            juce::Colour(0xfff0f0f0), centreX - knobRadius * 0.6f, centreY - knobRadius * 0.6f,  // Light top-left
            juce::Colour(0xff808080), centreX + knobRadius * 0.4f, centreY + knobRadius * 0.4f,   // Dark bottom-right
            false);  // Diagonal gradient for 3D effect
        metalGradient.addColour(0.3, juce::Colour(0xffe0e0e0));
        metalGradient.addColour(0.7, juce::Colour(0xffa0a0a0));
        
        g.setGradientFill(metalGradient);
        g.fillEllipse(knobBounds);
        
        // Add metallic rim (matching vertical slider style)
        g.setColour(juce::Colour(0xff404040));
        g.drawEllipse(knobBounds, 1.0f);
        
        // Draw center indicator line (like the reference image)
        g.setColour(juce::Colour(0xff404040));  // Dark gray indicator
        auto indicatorLength = knobRadius * 0.7f;
        auto indicatorThickness = 2.0f;
        
        auto indicatorStartX = centreX;
        auto indicatorStartY = centreY;
        auto indicatorEndX = centreX + std::cos(angle) * indicatorLength;
        auto indicatorEndY = centreY + std::sin(angle) * indicatorLength;
        
        g.drawLine(indicatorStartX, indicatorStartY, indicatorEndX, indicatorEndY, indicatorThickness);
    }
    
    void drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                 const juce::String& text,
                                 const juce::Justification& position,
                                 juce::GroupComponent& group) override
    {
        juce::ignoreUnused(position, group);
        
        const float indent = 3.0f;
        auto cs = 8.0f; // Corner size for rounded rectangle
        
        juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
        auto textH = f.getHeight();
        auto bottomBandHeight = textH + 8.0f; // Thick colored bottom band for title
        
        // Main content area (dark background)
        juce::Path mainArea;
        auto x = indent;
        auto y = indent;
        auto w = (float)width - indent * 2.0f;
        auto h = (float)height - indent * 2.0f - bottomBandHeight;
        
        // Create main area with rounded top corners only
        mainArea.addRoundedRectangle(x, y, w, h, cs, cs, true, true, false, false);
        
        // Fill main area with dark color
        g.setColour(juce::Colour(0xff1a1a1a)); // Very dark gray background
        g.fillPath(mainArea);
        
        // Create colored bottom band (like "TONE SOURCE ONE" band)
        auto bottomBandY = y + h;
        juce::Path bottomBand;
        bottomBand.addRoundedRectangle(x, bottomBandY, w, bottomBandHeight, cs, cs, false, false, true, true);
        
        // Choose color based on group text
        juce::Colour groupColour;
        if (text == "LFO" || text == "PM" || text == "Tape Delay")
        {
            groupColour = juce::Colour(0xff7A9A5A); // Green color matching image for LFO, FM, and delay groups
        }
        else if (text == "Noise Generator" || text == "Filter 1" || text == "Filter 2" || text == "Plate Reverb")
        {
            groupColour = juce::Colour(0xff9bb3c7); // Light blue color for noise generator, filter groups, and reverb
        }
        else if (text == "Envelope" || text == "Routing" || text == "Mod Env 1" || text == "Mod Env 2")
        {
            groupColour = juce::Colour(0xffc5c2a3); // Tan/beige color for envelope, routing, and modulation envelopes
        }
        else if (text.contains("Oscillator") || text == "Wavefolder Distortion")
        {
            groupColour = juce::Colour(0xffc77b7b); // Reddish-pink color for oscillator groups and wavefolder distortion
        }
        else
        {
            groupColour = juce::Colour(0xffcc4466); // Default darker red for other groups
        }
        
        // Fill bottom band with chosen color
        g.setColour(groupColour);
        g.fillPath(bottomBand);
        
        // Draw border around entire component with same color
        juce::Path border;
        border.addRoundedRectangle(x, y, w, h + bottomBandHeight, cs);
        g.setColour(groupColour);
        g.strokePath(border, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved));
        
        // Draw the black title text centered in the colored bottom band
        g.setColour(juce::Colour(0xff000000)); // Black text like reference image
        g.setFont(f);
        g.drawText(text,
                  juce::roundToInt(x), juce::roundToInt(bottomBandY),
                  juce::roundToInt(w), juce::roundToInt(bottomBandHeight),
                  juce::Justification::centred, true);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            // Professional mixer-style vertical slider
            auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
            
            // Draw scale markings first (behind everything)
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.setFont(7.0f);
            
            auto scaleArea = bounds.reduced(4.0f);
            auto scaleHeight = scaleArea.getHeight() - 16.0f; // Leave space for thumb
            auto scaleTop = scaleArea.getY() + 8.0f;
            
            // Draw horizontal scale lines and numbers (professional mixer style)
            for (int i = 0; i <= 10; ++i)
            {
                float proportion = i / 10.0f;
                float lineY = scaleTop + scaleHeight * (1.0f - proportion); // 10 at top, 0 at bottom
                
                // Main scale lines (longer for major ticks)
                bool isMajorTick = (i % 2 == 0);
                float lineLength = isMajorTick ? 6.0f : 3.0f;
                float lineStartX = bounds.getCentreX() - lineLength * 0.5f;
                float lineEndX = bounds.getCentreX() + lineLength * 0.5f;
                
                g.setColour(juce::Colours::white.withAlpha(isMajorTick ? 0.7f : 0.4f));
                g.drawLine(lineStartX, lineY, lineEndX, lineY, 0.8f);
                
                // Draw numbers for major ticks only
                if (isMajorTick && i > 0)
                {
                    g.setColour(juce::Colours::white.withAlpha(0.6f));
                    juce::String numberText = juce::String(i);
                    auto textBounds = juce::Rectangle<float>(bounds.getRight() + 2.0f, lineY - 4.0f, 10.0f, 8.0f);
                    g.drawText(numberText, textBounds.toNearestInt(), juce::Justification::centredLeft);
                }
            }
            
            // Draw "MIN" and "MAX" labels
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.setFont(6.0f);
            auto minBounds = juce::Rectangle<float>(bounds.getX() - 18.0f, scaleTop + scaleHeight - 4.0f, 16.0f, 8.0f);
            g.drawText("MIN", minBounds.toNearestInt(), juce::Justification::centredRight);
            
            auto maxBounds = juce::Rectangle<float>(bounds.getX() - 18.0f, scaleTop - 4.0f, 16.0f, 8.0f);
            g.drawText("MAX", maxBounds.toNearestInt(), juce::Justification::centredRight);
            
            // Draw the slider track (recessed channel)
            auto trackWidth = 4.0f;
            auto trackBounds = bounds.withSizeKeepingCentre(trackWidth, scaleHeight);
            trackBounds = trackBounds.withY(scaleTop);
            
            // Dark recessed track with subtle gradient
            juce::ColourGradient trackGradient(
                juce::Colour(0xff0a0a0a), trackBounds.getCentreX() - trackWidth * 0.5f, trackBounds.getCentreY(),
                juce::Colour(0xff1a1a1a), trackBounds.getCentreX() + trackWidth * 0.5f, trackBounds.getCentreY(),
                false
            );
            g.setGradientFill(trackGradient);
            g.fillRoundedRectangle(trackBounds, trackWidth * 0.5f);
            
            // Track border
            g.setColour(juce::Colour(0xff333333));
            g.drawRoundedRectangle(trackBounds, trackWidth * 0.5f, 0.5f);
            
            // Draw professional metallic thumb
            auto thumbRadius = 10.0f; // Increased from 8.0f for slightly larger thumbs
            
            // Constrain thumb position within track bounds (account for thumb radius)
            auto trackTop = trackBounds.getY() + thumbRadius;
            auto trackBottom = trackBounds.getBottom() - thumbRadius;
            auto thumbY = juce::jlimit(trackTop, trackBottom, sliderPos);
            
            auto thumbBounds = juce::Rectangle<float>(bounds.getCentreX() - thumbRadius, thumbY - thumbRadius, 
                                                    thumbRadius * 2.0f, thumbRadius * 2.0f);
            
            // Thumb shadow for depth
            auto shadowBounds = thumbBounds.translated(1.0f, 1.0f);
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.fillEllipse(shadowBounds);
            
            // Main thumb body with metallic gradient
            juce::ColourGradient thumbGradient(
                juce::Colour(0xfff0f0f0), thumbBounds.getCentreX() - thumbRadius * 0.6f, thumbBounds.getCentreY() - thumbRadius * 0.6f,
                juce::Colour(0xff808080), thumbBounds.getCentreX() + thumbRadius * 0.4f, thumbBounds.getCentreY() + thumbRadius * 0.4f,
                false
            );
            thumbGradient.addColour(0.3, juce::Colour(0xffe0e0e0));
            thumbGradient.addColour(0.7, juce::Colour(0xffa0a0a0));
            
            g.setGradientFill(thumbGradient);
            g.fillEllipse(thumbBounds);
            
            // Thumb border
            g.setColour(juce::Colour(0xff404040));
            g.drawEllipse(thumbBounds, 1.0f);
            
            // Thumb highlight
            auto highlightBounds = thumbBounds.reduced(2.0f);
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.fillEllipse(highlightBounds.removeFromTop(highlightBounds.getHeight() * 0.4f));
            
            // Add red LED indicators along the side (like knob LEDs)
            const int numLEDs = 11; // Number of LEDs along the track
            const float ledRadius = 1.5f;
            const float ledOffsetX = -20.0f; // Distance from center of track to the left
            
            // Calculate slider position as 0-1 value
            float normalizedSliderPos = (sliderPos - trackTop) / (trackBottom - trackTop);
            normalizedSliderPos = 1.0f - juce::jlimit(0.0f, 1.0f, normalizedSliderPos); // Invert so 0 is at bottom
            
            for (int i = 0; i < numLEDs; ++i)
            {
                float ledProportion = i / float(numLEDs - 1);
                float ledY = trackTop + (trackBottom - trackTop) * (1.0f - ledProportion); // 0 at bottom, 1 at top
                float ledX = bounds.getCentreX() + ledOffsetX;
                
                // Calculate if this LED should be lit (traditional level meter)
                bool isLit = ledProportion <= normalizedSliderPos;
                
                // Calculate distance from this LED to the current slider position (for gradient effect)
                float distanceFromSlider = std::abs(normalizedSliderPos - ledProportion);
                
                // Create gradient effect - shorter above slider, normal below
                float gradientRadiusBelow = 0.18f; // Full gradient below slider (1.8 LEDs)
                float gradientRadiusAbove = 0.12f; // Shorter gradient above slider (1.2 LEDs)
                
                bool isAboveSlider = ledProportion > normalizedSliderPos;
                float effectiveGradientRadius = isAboveSlider ? gradientRadiusAbove : gradientRadiusBelow;
                bool inGradient = distanceFromSlider <= effectiveGradientRadius;
                
                // Draw black outer ring for all LEDs
                g.setColour(juce::Colour(0xff000000));
                g.fillEllipse(ledX - ledRadius * 1.1f, ledY - ledRadius * 1.1f, ledRadius * 2.2f, ledRadius * 2.2f);
                
                if (isLit || inGradient)
                {
                    float brightness;
                    
                    if (inGradient)
                    {
                        // Within gradient radius - bright gradient effect
                        brightness = juce::jlimit(0.5f, 1.0f, 1.0f - (distanceFromSlider / effectiveGradientRadius) * 0.5f);
                    }
                    else if (isLit)
                    {
                        // Normal lit LED outside gradient - even brighter so they're clearly visible
                        brightness = 0.65f;
                    }
                    else
                    {
                        // Should not reach here, but safety fallback
                        brightness = 0.2f;
                    }
                    
                    // Scale the red color by brightness
                    juce::uint8 redValue = (juce::uint8)(255 * brightness);
                    juce::uint8 greenValue = (juce::uint8)(34 * brightness); // Keep the orange tint
                    juce::Colour ledColour = juce::Colour(redValue, greenValue, (juce::uint8)0, (juce::uint8)255);
                    
                    g.setColour(ledColour);
                    g.fillEllipse(ledX - ledRadius, ledY - ledRadius, ledRadius * 2.0f, ledRadius * 2.0f);
                    
                    // Add glow effect - stronger for brighter LEDs
                    float glowAlpha = brightness * 0.6f;
                    g.setColour(juce::Colour(redValue, greenValue, (juce::uint8)0, (juce::uint8)(255 * glowAlpha)));
                    g.fillEllipse(ledX - ledRadius * 1.5f, ledY - ledRadius * 1.5f, ledRadius * 3.0f, ledRadius * 3.0f);
                }
                else
                {
                    // Unlit LED - dark red color
                    g.setColour(juce::Colour(0xff331111));
                    g.fillEllipse(ledX - ledRadius, ledY - ledRadius, ledRadius * 2.0f, ledRadius * 2.0f);
                }
            }
        }
        else
        {
            // Fall back to default for other slider styles
            juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }
    
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                     int buttonX, int buttonY, int buttonW, int buttonH,
                     juce::ComboBox& comboBox) override
    {
        juce::ignoreUnused(buttonX, buttonY, buttonW, buttonH, comboBox);
        
        auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        
        // Retro 80s black plastic dropdown styling with 3D depth (matching waveform selectors)
        if (isButtonDown)
        {
            // PUSHED DOWN state (when clicked)
            // Dark recessed background
            g.setColour(juce::Colour(0xff1a1a1a)); // Very dark background
            g.fillRoundedRectangle(bounds, 2.0f);
            
            // Dark shadow on top and left (inset look)
            g.setColour(juce::Colour(0xff0d0d0d)); // Even darker shadow
            g.drawLine(bounds.getX(), bounds.getY(), 
                      bounds.getRight(), bounds.getY(), 1.0f); // Top shadow
            g.drawLine(bounds.getX(), bounds.getY(), 
                      bounds.getX(), bounds.getBottom(), 1.0f); // Left shadow
            
            // Subtle highlight on bottom and right (for depth)
            g.setColour(juce::Colour(0xff404040));
            g.drawLine(bounds.getX(), bounds.getBottom() - 1, 
                      bounds.getRight(), bounds.getBottom() - 1, 1.0f); // Bottom highlight
            g.drawLine(bounds.getRight() - 1, bounds.getY(), 
                      bounds.getRight() - 1, bounds.getBottom(), 1.0f); // Right highlight
        }
        else
        {
            // RAISED state (normal)
            // Black plastic background
            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRoundedRectangle(bounds, 2.0f);
            
            // Bright highlight on top and left (raised black plastic look)
            g.setColour(juce::Colour(0xff707070)); // Medium grey highlight for black plastic
            g.drawLine(bounds.getX(), bounds.getY(), 
                      bounds.getRight(), bounds.getY(), 1.0f); // Top highlight
            g.drawLine(bounds.getX(), bounds.getY(), 
                      bounds.getX(), bounds.getBottom(), 1.0f); // Left highlight
            
            // Dark shadow on bottom and right (for 3D depth)
            g.setColour(juce::Colour(0xff0a0a0a)); // Very dark shadow for black plastic
            g.drawLine(bounds.getX(), bounds.getBottom() - 1, 
                      bounds.getRight(), bounds.getBottom() - 1, 1.0f); // Bottom shadow
            g.drawLine(bounds.getRight() - 1, bounds.getY(), 
                      bounds.getRight() - 1, bounds.getBottom(), 1.0f); // Right shadow
        }
        
        // Draw dropdown arrow on the right side - use original component dimensions
        auto originalBounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        
        // Calculate arrow size based on component height, but keep it reasonable
        auto arrowHeight = juce::jmin(originalBounds.getHeight() * 0.3f, 8.0f); // Max 8px high, or 30% of component height
        auto arrowWidth = arrowHeight * 1.2f; // Slightly wider than tall
        
        // Position arrow in the rightmost area - always use a fixed width area from the right edge
        auto arrowAreaWidth = juce::jmax(20.0f, arrowWidth + 8.0f); // Ensure enough space around arrow
        auto arrowAreaX = originalBounds.getRight() - arrowAreaWidth;
        auto arrowBounds = juce::Rectangle<float>(arrowAreaX, originalBounds.getY(), arrowAreaWidth, originalBounds.getHeight());
        
        // Center the arrow in the arrow area
        auto arrowRect = arrowBounds.withSizeKeepingCentre(arrowWidth, arrowHeight);
        
        auto arrowColor = juce::Colour(0xffa0a0a0); // Light grey arrow
        
        juce::Path arrowPath;
        arrowPath.startNewSubPath(arrowRect.getX(), arrowRect.getY());
        arrowPath.lineTo(arrowRect.getCentreX(), arrowRect.getBottom());
        arrowPath.lineTo(arrowRect.getRight(), arrowRect.getY());
        arrowPath.closeSubPath();
        
        g.setColour(arrowColor);
        g.fillPath(arrowPath);
    }
    
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        // Black plastic popup background with subtle border
        auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        
        // Dark background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRect(bounds);
        
        // Subtle border for definition
        g.setColour(juce::Colour(0xff505050));
        g.drawRect(bounds, 1.0f);
    }
    
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                          bool isSeparator, bool isActive, bool isHighlighted,
                          bool isTicked, bool hasSubMenu, const juce::String& text,
                          const juce::String& shortcutKeyText,
                          const juce::Drawable* icon, const juce::Colour* textColour) override
    {
        juce::ignoreUnused(isSeparator, isTicked, hasSubMenu, shortcutKeyText, icon);
        
        if (isHighlighted && isActive)
        {
            // Highlighted item background - slightly lighter black plastic
            g.setColour(juce::Colour(0xff404040));
            g.fillRect(area);
        }
        
        // Text color - white when highlighted, light grey otherwise
        auto colour = (isHighlighted && isActive) ? juce::Colours::white : juce::Colour(0xffa0a0a0);
        if (textColour != nullptr)
            colour = *textColour;
            
        g.setColour(colour);
        g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f)));
        
        auto textArea = area.reduced(8, 0); // Padding from sides
        g.drawText(text, textArea, juce::Justification::centredLeft);
    }
};

//==============================================================================
// WaveformSelector Implementation

FreOscEditor::WaveformSelector::WaveformSelector()
{
    // Enable mouse tracking for hover effects
    setMouseClickGrabsKeyboardFocus(false);
}

void FreOscEditor::WaveformSelector::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Vintage synth panel background - dark metal to complement black plastic buttons
    g.setColour(juce::Colour(0xff1e1e1e)); // Dark background for black plastic aesthetic
    g.fillRoundedRectangle(bounds.toFloat(), 2.0f);
    
    // Subtle recessed border
    g.setColour(juce::Colour(0xff0d0d0d)); // Very dark shadow
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 1.5f, 1.0f);
    
    // Draw each waveform button
    drawWaveform(g, sineButton, Sine, selectedWaveform == Sine);
    drawWaveform(g, squareButton, Square, selectedWaveform == Square);
    drawWaveform(g, sawButton, Sawtooth, selectedWaveform == Sawtooth);
    drawWaveform(g, triangleButton, Triangle, selectedWaveform == Triangle);
}

void FreOscEditor::WaveformSelector::drawWaveform(juce::Graphics& g, juce::Rectangle<int> area, Waveform waveform, bool isSelected)
{
    if (area.isEmpty()) return;
    
    // Vintage 80s synth button styling with 3D depth
    auto buttonBounds = area.toFloat().reduced(1.0f); // Small gap between buttons
    
    if (isSelected)
    {
        // PUSHED DOWN button (selected state)
        // Dark recessed background
        g.setColour(juce::Colour(0xff1a1a1a)); // Very dark background
        g.fillRoundedRectangle(buttonBounds, 1.0f);
        
        // Dark shadow on top and left (inset look)
        g.setColour(juce::Colour(0xff0d0d0d)); // Even darker shadow
        g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                  buttonBounds.getRight(), buttonBounds.getY(), 1.0f); // Top shadow
        g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                  buttonBounds.getX(), buttonBounds.getBottom(), 1.0f); // Left shadow
        
        // Subtle highlight on bottom and right (for depth)
        g.setColour(juce::Colour(0xff404040));
        g.drawLine(buttonBounds.getX(), buttonBounds.getBottom() - 1, 
                  buttonBounds.getRight(), buttonBounds.getBottom() - 1, 1.0f); // Bottom highlight
        g.drawLine(buttonBounds.getRight() - 1, buttonBounds.getY(), 
                  buttonBounds.getRight() - 1, buttonBounds.getBottom(), 1.0f); // Right highlight
    }
    else
    {
        // RAISED button (unselected state)
        // Black plastic background with subtle variation for hover
        auto bgColor = (isHovering && hoveredWaveform == waveform) ? 
                      juce::Colour(0xff383838) : juce::Colour(0xff2a2a2a);
        
        g.setColour(bgColor);
        g.fillRoundedRectangle(buttonBounds, 1.0f);
        
        // Bright highlight on top and left (raised black plastic look)
        g.setColour(juce::Colour(0xff707070)); // Medium grey highlight for black plastic
        g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                  buttonBounds.getRight(), buttonBounds.getY(), 1.0f); // Top highlight
        g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                  buttonBounds.getX(), buttonBounds.getBottom(), 1.0f); // Left highlight
        
        // Dark shadow on bottom and right (for 3D depth)
        g.setColour(juce::Colour(0xff0a0a0a)); // Very dark shadow for black plastic
        g.drawLine(buttonBounds.getX(), buttonBounds.getBottom() - 1, 
                  buttonBounds.getRight(), buttonBounds.getBottom() - 1, 1.0f); // Bottom shadow
        g.drawLine(buttonBounds.getRight() - 1, buttonBounds.getY(), 
                  buttonBounds.getRight() - 1, buttonBounds.getBottom(), 1.0f); // Right shadow
    }
    
    // Draw waveform with high contrast colors for black plastic buttons
    // Selected buttons have bright white waveforms, unselected have light grey waveforms
    g.setColour(isSelected ? juce::Colours::white : juce::Colour(0xffa0a0a0));
    
    // Adjust waveform area based on button state (pressed buttons have offset content)
    auto waveArea = area.reduced(isSelected ? 6 : 4); // More padding when pressed
    if (isSelected)
    {
        // Offset the waveform down and right when button is pressed
        waveArea = waveArea.translated(1, 1);
    }
    
    auto centreY = static_cast<float>(waveArea.getCentreY());
    auto amplitude = waveArea.getHeight() * 0.25f; // Slightly smaller for vintage look
    
    juce::Path wavePath;
    
    switch (waveform)
    {
        case Sine:
        {
            // Draw sine wave
            wavePath.startNewSubPath(static_cast<float>(waveArea.getX()), centreY);
            for (int x = waveArea.getX(); x < waveArea.getRight(); ++x)
            {
                float progress = (x - waveArea.getX()) / static_cast<float>(waveArea.getWidth());
                float y = centreY + amplitude * std::sin(progress * juce::MathConstants<float>::twoPi * 2.0f);
                wavePath.lineTo(static_cast<float>(x), y);
            }
            break;
        }
        case Square:
        {
            // Draw square wave
            float quarterWidth = waveArea.getWidth() * 0.25f;
            float x = static_cast<float>(waveArea.getX());
            
            wavePath.startNewSubPath(x, centreY + amplitude);
            wavePath.lineTo(x + quarterWidth, centreY + amplitude);
            wavePath.lineTo(x + quarterWidth, centreY - amplitude);
            wavePath.lineTo(x + quarterWidth * 3, centreY - amplitude);
            wavePath.lineTo(x + quarterWidth * 3, centreY + amplitude);
            wavePath.lineTo(static_cast<float>(waveArea.getRight()), centreY + amplitude);
            break;
        }
        case Sawtooth:
        {
            // Draw sawtooth wave
            float halfWidth = waveArea.getWidth() * 0.5f;
            float x = static_cast<float>(waveArea.getX());
            
            wavePath.startNewSubPath(x, centreY + amplitude);
            wavePath.lineTo(x + halfWidth, centreY - amplitude);
            wavePath.lineTo(x + halfWidth, centreY + amplitude);
            wavePath.lineTo(static_cast<float>(waveArea.getRight()), centreY - amplitude);
            break;
        }
        case Triangle:
        {
            // Draw triangle wave
            float quarterWidth = waveArea.getWidth() * 0.25f;
            float x = static_cast<float>(waveArea.getX());
            
            wavePath.startNewSubPath(x, centreY);
            wavePath.lineTo(x + quarterWidth, centreY - amplitude);
            wavePath.lineTo(x + quarterWidth * 3, centreY + amplitude);
            wavePath.lineTo(static_cast<float>(waveArea.getRight()), centreY);
            break;
        }
    }
    
    // Use thicker lines for vintage visibility
    float strokeWidth = isSelected ? 2.0f : 1.8f;
    g.strokePath(wavePath, juce::PathStrokeType(strokeWidth));
}

void FreOscEditor::WaveformSelector::mouseDown(const juce::MouseEvent& event)
{
    auto pos = event.getPosition();
    
    Waveform newWaveform = selectedWaveform; // Keep current if no valid click
    bool foundValidClick = false;
    
    // Check each button area more precisely
    if (sineButton.contains(pos))
    {
        newWaveform = Sine;
        foundValidClick = true;
    }
    else if (squareButton.contains(pos))
    {
        newWaveform = Square;
        foundValidClick = true;
    }
    else if (sawButton.contains(pos))
    {
        newWaveform = Sawtooth;
        foundValidClick = true;
    }
    else if (triangleButton.contains(pos))
    {
        newWaveform = Triangle;
        foundValidClick = true;
    }
    
    // Only change selection if we clicked on a valid button AND it's different
    // This prevents deselection and ensures radio button behavior
    if (foundValidClick && newWaveform != selectedWaveform)
    {
        selectedWaveform = newWaveform;
        repaint();
        
        if (onWaveformChanged)
            onWaveformChanged(static_cast<int>(selectedWaveform));
    }
}

void FreOscEditor::WaveformSelector::mouseMove(const juce::MouseEvent& event)
{
    auto pos = event.getPosition();
    Waveform newHover = Sine;
    bool wasHovering = isHovering;
    
    if (sineButton.contains(pos))
    {
        newHover = Sine;
        isHovering = true;
    }
    else if (squareButton.contains(pos))
    {
        newHover = Square;
        isHovering = true;
    }
    else if (sawButton.contains(pos))
    {
        newHover = Sawtooth;
        isHovering = true;
    }
    else if (triangleButton.contains(pos))
    {
        newHover = Triangle;
        isHovering = true;
    }
    else
    {
        isHovering = false;
    }
    
    if (isHovering != wasHovering || (isHovering && newHover != hoveredWaveform))
    {
        hoveredWaveform = newHover;
        repaint();
    }
}

void FreOscEditor::WaveformSelector::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    if (isHovering)
    {
        isHovering = false;
        repaint();
    }
}

void FreOscEditor::WaveformSelector::resized()
{
    auto bounds = getLocalBounds().reduced(1); // Minimal outer padding
    auto buttonWidth = bounds.getWidth() / 4;
    
    // Create buttons with minimal padding for better hit detection
    sineButton = bounds.removeFromLeft(buttonWidth);
    squareButton = bounds.removeFromLeft(buttonWidth);
    sawButton = bounds.removeFromLeft(buttonWidth);
    triangleButton = bounds; // Last button gets remaining space
}

void FreOscEditor::WaveformSelector::setSelectedWaveform(Waveform waveform)
{
    if (selectedWaveform != waveform)
    {
        selectedWaveform = waveform;
        repaint();
    }
}

juce::Rectangle<int> FreOscEditor::WaveformSelector::getButtonForWaveform(Waveform waveform) const
{
    switch (waveform)
    {
        case Sine: return sineButton;
        case Square: return squareButton;
        case Sawtooth: return sawButton;
        case Triangle: return triangleButton;
        default: return {};
    }
}

//==============================================================================
// WaveformSelectorAttachment Implementation

FreOscEditor::WaveformSelectorAttachment::WaveformSelectorAttachment(
    juce::AudioProcessorValueTreeState& state, 
    const juce::String& parameterID, 
    WaveformSelector& selector)
    : valueTreeState(state), paramID(parameterID), waveformSelector(selector)
{
    // Add listener to parameter changes
    valueTreeState.addParameterListener(paramID, this);
    
    // Set initial value from parameter
    // For AudioParameterChoice, we need to get the actual choice index
    if (auto* param = valueTreeState.getParameter(paramID))
    {
        // Try to cast to AudioParameterChoice to get the proper index
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        {
            auto waveform = choiceParam->getIndex();
            waveformSelector.setSelectedWaveform(waveform);
        }
        else
        {
            // Fallback to raw value conversion
            auto paramValue = param->getValue();
            auto waveform = static_cast<int>(paramValue);
            waveformSelector.setSelectedWaveform(waveform);
        }
    }
    
    // Listen for selector changes
    waveformSelector.onWaveformChanged = [this](int waveform) { selectorChanged(waveform); };
}

FreOscEditor::WaveformSelectorAttachment::~WaveformSelectorAttachment()
{
    valueTreeState.removeParameterListener(paramID, this);
    waveformSelector.onWaveformChanged = nullptr;
}

void FreOscEditor::WaveformSelectorAttachment::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == paramID)
    {
        // For AudioParameterChoice, newValue should be the choice index
        auto waveform = static_cast<int>(newValue + 0.5f); // Add 0.5 for proper rounding
        waveformSelector.setSelectedWaveform(waveform);
    }
}

void FreOscEditor::WaveformSelectorAttachment::selectorChanged(int waveform)
{
    if (auto* param = valueTreeState.getParameter(paramID))
    {
        // For AudioParameterChoice, try to set using the proper method
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        {
            *choiceParam = waveform; // This uses the assignment operator which properly sets the index
        }
        else
        {
            // Fallback to normalized value
            float rawIndex = static_cast<float>(waveform);
            param->setValueNotifyingHost(rawIndex);
        }
    }
}

//==============================================================================
// LFOWaveformSelector Implementation

FreOscEditor::LFOWaveformSelector::LFOWaveformSelector()
{
    // Enable mouse tracking for hover effects
    setMouseClickGrabsKeyboardFocus(false);
}

void FreOscEditor::LFOWaveformSelector::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Vintage synth panel background - dark metal to complement black plastic buttons
    g.setColour(juce::Colour(0xff1e1e1e)); // Dark background for black plastic aesthetic
    g.fillRoundedRectangle(bounds.toFloat(), 2.0f);
    
    // Subtle recessed border
    g.setColour(juce::Colour(0xff0d0d0d)); // Very dark shadow
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 1.5f, 1.0f);
    
    // Draw each LFO waveform button
    drawWaveform(g, sineButton, LFO_Sine, selectedWaveform == LFO_Sine);
    drawWaveform(g, triangleButton, LFO_Triangle, selectedWaveform == LFO_Triangle);
    drawWaveform(g, sawButton, LFO_Sawtooth, selectedWaveform == LFO_Sawtooth);
    drawWaveform(g, squareButton, LFO_Square, selectedWaveform == LFO_Square);
    drawWaveform(g, randomButton, LFO_Random, selectedWaveform == LFO_Random);
}

void FreOscEditor::LFOWaveformSelector::drawWaveform(juce::Graphics& g, juce::Rectangle<int> area, LFOWaveform waveform, bool isSelected)
{
    if (area.isEmpty()) return;
    
    // Vintage 80s synth button styling with 3D depth
    auto buttonBounds = area.toFloat().reduced(1.0f); // Small gap between buttons
    
    if (isSelected)
    {
        // PUSHED DOWN button (selected state)
        g.setColour(juce::Colour(0xff1a1a1a)); // Very dark background
        g.fillRoundedRectangle(buttonBounds, 1.0f);
        
        // Dark shadow on top and left (inset look)
        g.setColour(juce::Colour(0xff0d0d0d)); // Even darker shadow
        g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                  buttonBounds.getRight(), buttonBounds.getY(), 1.0f); // Top shadow
        g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                  buttonBounds.getX(), buttonBounds.getBottom(), 1.0f); // Left shadow
        
        // Subtle highlight on bottom and right (for depth)
        g.setColour(juce::Colour(0xff404040));
        g.drawLine(buttonBounds.getX(), buttonBounds.getBottom() - 1, 
                  buttonBounds.getRight(), buttonBounds.getBottom() - 1, 1.0f); // Bottom highlight
        g.drawLine(buttonBounds.getRight() - 1, buttonBounds.getY(), 
                  buttonBounds.getRight() - 1, buttonBounds.getBottom(), 1.0f); // Right highlight
    }
    else
    {
        // RAISED button (unselected state)
        auto bgColor = (isHovering && hoveredWaveform == waveform) ? 
                      juce::Colour(0xff383838) : juce::Colour(0xff2a2a2a);
        
        g.setColour(bgColor);
        g.fillRoundedRectangle(buttonBounds, 1.0f);
        
        // Bright highlight on top and left (raised black plastic look)
        g.setColour(juce::Colour(0xff707070)); // Medium grey highlight for black plastic
        g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                  buttonBounds.getRight(), buttonBounds.getY(), 1.0f); // Top highlight
        g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                  buttonBounds.getX(), buttonBounds.getBottom(), 1.0f); // Left highlight
        
        // Dark shadow on bottom and right (for 3D depth)
        g.setColour(juce::Colour(0xff0a0a0a)); // Very dark shadow for black plastic
        g.drawLine(buttonBounds.getX(), buttonBounds.getBottom() - 1, 
                  buttonBounds.getRight(), buttonBounds.getBottom() - 1, 1.0f); // Bottom shadow
        g.drawLine(buttonBounds.getRight() - 1, buttonBounds.getY(), 
                  buttonBounds.getRight() - 1, buttonBounds.getBottom(), 1.0f); // Right shadow
    }
    
    // Draw waveform with high contrast colors for black plastic buttons
    g.setColour(isSelected ? juce::Colours::white : juce::Colour(0xffa0a0a0));
    
    // Adjust waveform area based on button state (pressed buttons have offset content)
    auto waveArea = area.reduced(isSelected ? 6 : 4); // More padding when pressed
    if (isSelected)
    {
        // Offset the waveform down and right when button is pressed
        waveArea = waveArea.translated(1, 1);
    }
    
    auto centreY = static_cast<float>(waveArea.getCentreY());
    auto amplitude = waveArea.getHeight() * 0.25f; // Slightly smaller for vintage look
    
    juce::Path wavePath;
    
    switch (waveform)
    {
        case LFO_Sine:
        {
            // Draw sine wave
            wavePath.startNewSubPath(static_cast<float>(waveArea.getX()), centreY);
            for (int x = waveArea.getX(); x < waveArea.getRight(); ++x)
            {
                float progress = (x - waveArea.getX()) / static_cast<float>(waveArea.getWidth());
                float y = centreY + amplitude * std::sin(progress * juce::MathConstants<float>::twoPi * 2.0f);
                wavePath.lineTo(static_cast<float>(x), y);
            }
            break;
        }
        case LFO_Triangle:
        {
            // Draw triangle wave
            float quarterWidth = waveArea.getWidth() * 0.25f;
            float x = static_cast<float>(waveArea.getX());
            
            wavePath.startNewSubPath(x, centreY);
            wavePath.lineTo(x + quarterWidth, centreY - amplitude);
            wavePath.lineTo(x + quarterWidth * 3, centreY + amplitude);
            wavePath.lineTo(static_cast<float>(waveArea.getRight()), centreY);
            break;
        }
        case LFO_Sawtooth:
        {
            // Draw sawtooth wave
            float halfWidth = waveArea.getWidth() * 0.5f;
            float x = static_cast<float>(waveArea.getX());
            
            wavePath.startNewSubPath(x, centreY + amplitude);
            wavePath.lineTo(x + halfWidth, centreY - amplitude);
            wavePath.lineTo(x + halfWidth, centreY + amplitude);
            wavePath.lineTo(static_cast<float>(waveArea.getRight()), centreY - amplitude);
            break;
        }
        case LFO_Square:
        {
            // Draw square wave
            float quarterWidth = waveArea.getWidth() * 0.25f;
            float x = static_cast<float>(waveArea.getX());
            
            wavePath.startNewSubPath(x, centreY + amplitude);
            wavePath.lineTo(x + quarterWidth, centreY + amplitude);
            wavePath.lineTo(x + quarterWidth, centreY - amplitude);
            wavePath.lineTo(x + quarterWidth * 3, centreY - amplitude);
            wavePath.lineTo(x + quarterWidth * 3, centreY + amplitude);
            wavePath.lineTo(static_cast<float>(waveArea.getRight()), centreY + amplitude);
            break;
        }
        case LFO_Random:
        {
            // Draw random/noise representation - jagged line
            wavePath.startNewSubPath(static_cast<float>(waveArea.getX()), centreY);
            int numPoints = 8;
            for (int i = 1; i <= numPoints; ++i)
            {
                float x = static_cast<float>(waveArea.getX()) + (static_cast<float>(i * waveArea.getWidth()) / static_cast<float>(numPoints));
                float randomY = centreY + amplitude * (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f);
                wavePath.lineTo(x, randomY);
            }
            break;
        }
    }
    
    // Use thicker lines for vintage visibility
    float strokeWidth = isSelected ? 2.0f : 1.8f;
    g.strokePath(wavePath, juce::PathStrokeType(strokeWidth));
}

void FreOscEditor::LFOWaveformSelector::mouseDown(const juce::MouseEvent& event)
{
    auto pos = event.getPosition();
    
    LFOWaveform newWaveform = selectedWaveform; // Keep current if no valid click
    bool foundValidClick = false;
    
    // Check each button area more precisely
    if (sineButton.contains(pos))
    {
        newWaveform = LFO_Sine;
        foundValidClick = true;
    }
    else if (triangleButton.contains(pos))
    {
        newWaveform = LFO_Triangle;
        foundValidClick = true;
    }
    else if (sawButton.contains(pos))
    {
        newWaveform = LFO_Sawtooth;
        foundValidClick = true;
    }
    else if (squareButton.contains(pos))
    {
        newWaveform = LFO_Square;
        foundValidClick = true;
    }
    else if (randomButton.contains(pos))
    {
        newWaveform = LFO_Random;
        foundValidClick = true;
    }
    
    // Only change selection if we clicked on a valid button AND it's different
    if (foundValidClick && newWaveform != selectedWaveform)
    {
        selectedWaveform = newWaveform;
        repaint();
        
        if (onWaveformChanged)
            onWaveformChanged(static_cast<int>(selectedWaveform));
    }
}

void FreOscEditor::LFOWaveformSelector::mouseMove(const juce::MouseEvent& event)
{
    auto pos = event.getPosition();
    LFOWaveform newHover = LFO_Sine;
    bool wasHovering = isHovering;
    
    if (sineButton.contains(pos))
    {
        newHover = LFO_Sine;
        isHovering = true;
    }
    else if (triangleButton.contains(pos))
    {
        newHover = LFO_Triangle;
        isHovering = true;
    }
    else if (sawButton.contains(pos))
    {
        newHover = LFO_Sawtooth;
        isHovering = true;
    }
    else if (squareButton.contains(pos))
    {
        newHover = LFO_Square;
        isHovering = true;
    }
    else if (randomButton.contains(pos))
    {
        newHover = LFO_Random;
        isHovering = true;
    }
    else
    {
        isHovering = false;
    }
    
    if (isHovering != wasHovering || (isHovering && newHover != hoveredWaveform))
    {
        hoveredWaveform = newHover;
        repaint();
    }
}

void FreOscEditor::LFOWaveformSelector::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    if (isHovering)
    {
        isHovering = false;
        repaint();
    }
}

void FreOscEditor::LFOWaveformSelector::resized()
{
    auto bounds = getLocalBounds().reduced(1); // Minimal outer padding
    auto buttonWidth = bounds.getWidth() / 5; // 5 buttons for LFO waveforms
    
    // Create buttons with minimal padding for better hit detection
    sineButton = bounds.removeFromLeft(buttonWidth);
    triangleButton = bounds.removeFromLeft(buttonWidth);
    sawButton = bounds.removeFromLeft(buttonWidth);
    squareButton = bounds.removeFromLeft(buttonWidth);
    randomButton = bounds; // Last button gets remaining space
}

void FreOscEditor::LFOWaveformSelector::setSelectedWaveform(LFOWaveform waveform)
{
    if (selectedWaveform != waveform)
    {
        selectedWaveform = waveform;
        repaint();
    }
}

juce::Rectangle<int> FreOscEditor::LFOWaveformSelector::getButtonForWaveform(LFOWaveform waveform) const
{
    switch (waveform)
    {
        case LFO_Sine: return sineButton;
        case LFO_Triangle: return triangleButton;
        case LFO_Sawtooth: return sawButton;
        case LFO_Square: return squareButton;
        case LFO_Random: return randomButton;
        default: return {};
    }
}

//==============================================================================
// LFOWaveformSelectorAttachment Implementation

FreOscEditor::LFOWaveformSelectorAttachment::LFOWaveformSelectorAttachment(
    juce::AudioProcessorValueTreeState& state, 
    const juce::String& parameterID, 
    LFOWaveformSelector& selector)
    : valueTreeState(state), paramID(parameterID), lfoWaveformSelector(selector)
{
    // Add listener to parameter changes
    valueTreeState.addParameterListener(paramID, this);
    
    // Set initial value from parameter
    if (auto* param = valueTreeState.getParameter(paramID))
    {
        // Try to cast to AudioParameterChoice to get the proper index
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        {
            auto waveform = choiceParam->getIndex();
            lfoWaveformSelector.setSelectedWaveform(waveform);
        }
        else
        {
            // Fallback to raw value conversion
            auto paramValue = param->getValue();
            auto waveform = static_cast<int>(paramValue);
            lfoWaveformSelector.setSelectedWaveform(waveform);
        }
    }
    
    // Listen for selector changes
    lfoWaveformSelector.onWaveformChanged = [this](int waveform) { selectorChanged(waveform); };
}

FreOscEditor::LFOWaveformSelectorAttachment::~LFOWaveformSelectorAttachment()
{
    valueTreeState.removeParameterListener(paramID, this);
    lfoWaveformSelector.onWaveformChanged = nullptr;
}

void FreOscEditor::LFOWaveformSelectorAttachment::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == paramID)
    {
        // For AudioParameterChoice, newValue should be the choice index
        auto waveform = static_cast<int>(newValue + 0.5f); // Add 0.5 for proper rounding
        lfoWaveformSelector.setSelectedWaveform(waveform);
    }
}

void FreOscEditor::LFOWaveformSelectorAttachment::selectorChanged(int waveform)
{
    if (auto* param = valueTreeState.getParameter(paramID))
    {
        // For AudioParameterChoice, try to set using the proper method
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        {
            *choiceParam = waveform; // This uses the assignment operator which properly sets the index
        }
        else
        {
            // Fallback to normalized value
            float rawIndex = static_cast<float>(waveform);
            param->setValueNotifyingHost(rawIndex);
        }
    }
}

//==============================================================================
// OctaveSelector Implementation

FreOscEditor::OctaveSelector::OctaveSelector()
{
    // Enable mouse tracking for hover effects
    setMouseClickGrabsKeyboardFocus(false);
}

void FreOscEditor::OctaveSelector::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Vintage synth panel background - dark metal to complement black plastic buttons
    g.setColour(juce::Colour(0xff1e1e1e)); // Dark background for black plastic aesthetic
    g.fillRoundedRectangle(bounds.toFloat(), 2.0f);
    
    // Subtle recessed border
    g.setColour(juce::Colour(0xff0d0d0d)); // Very dark shadow
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 1.5f, 1.0f);
    
    // Draw each octave button
    drawOctaveButton(g, minus2Button, Minus2, selectedOctave == Minus2);
    drawOctaveButton(g, minus1Button, Minus1, selectedOctave == Minus1);
    drawOctaveButton(g, zeroButton, Zero, selectedOctave == Zero);
    drawOctaveButton(g, plus1Button, Plus1, selectedOctave == Plus1);
    drawOctaveButton(g, plus2Button, Plus2, selectedOctave == Plus2);
}

void FreOscEditor::OctaveSelector::drawOctaveButton(juce::Graphics& g, juce::Rectangle<int> area, OctaveValue octave, bool isSelected)
{
    if (area.isEmpty()) return;
    
    // Vintage 80s synth button styling with 3D depth (same as waveform buttons)
    auto buttonBounds = area.toFloat().reduced(1.0f); // Small gap between buttons
    
    if (isSelected)
    {
        // PUSHED DOWN button (selected state)
        // Dark recessed background
        g.setColour(juce::Colour(0xff1a1a1a)); // Very dark background
        g.fillRoundedRectangle(buttonBounds, 1.0f);
        
        // Dark shadow on top and left (inset look)
        g.setColour(juce::Colour(0xff0d0d0d)); // Even darker shadow
        g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                  buttonBounds.getRight(), buttonBounds.getY(), 1.0f); // Top shadow
        g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                  buttonBounds.getX(), buttonBounds.getBottom(), 1.0f); // Left shadow
        
        // Subtle highlight on bottom and right (for depth)
        g.setColour(juce::Colour(0xff404040));
        g.drawLine(buttonBounds.getX(), buttonBounds.getBottom() - 1, 
                  buttonBounds.getRight(), buttonBounds.getBottom() - 1, 1.0f); // Bottom highlight
        g.drawLine(buttonBounds.getRight() - 1, buttonBounds.getY(), 
                  buttonBounds.getRight() - 1, buttonBounds.getBottom(), 1.0f); // Right highlight
    }
    else
    {
        // RAISED button (unselected state)
        // Black plastic background with subtle variation for hover
        auto bgColor = (isHovering && hoveredOctave == octave) ? 
                      juce::Colour(0xff383838) : juce::Colour(0xff2a2a2a);
        
        g.setColour(bgColor);
        g.fillRoundedRectangle(buttonBounds, 1.0f);
        
        // Bright highlight on top and left (raised black plastic look)
        g.setColour(juce::Colour(0xff707070)); // Medium grey highlight for black plastic
        g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                  buttonBounds.getRight(), buttonBounds.getY(), 1.0f); // Top highlight
        g.drawLine(buttonBounds.getX(), buttonBounds.getY(), 
                  buttonBounds.getX(), buttonBounds.getBottom(), 1.0f); // Left highlight
        
        // Dark shadow on bottom and right (for 3D depth)
        g.setColour(juce::Colour(0xff0a0a0a)); // Very dark shadow for black plastic
        g.drawLine(buttonBounds.getX(), buttonBounds.getBottom() - 1, 
                  buttonBounds.getRight(), buttonBounds.getBottom() - 1, 1.0f); // Bottom shadow
        g.drawLine(buttonBounds.getRight() - 1, buttonBounds.getY(), 
                  buttonBounds.getRight() - 1, buttonBounds.getBottom(), 1.0f); // Right shadow
    }
    
    // Draw octave text
    auto textColor = isSelected ? juce::Colours::white : juce::Colour(0xffa0a0a0);
    g.setColour(textColor);
    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    
    // Adjust text area based on button state (pressed buttons have offset content)
    auto textArea = buttonBounds.reduced(2.0f);
    if (isSelected)
    {
        // Offset the text down and right when button is pressed
        textArea = textArea.translated(1.0f, 1.0f);
    }
    
    g.drawText(getOctaveText(octave), textArea.toNearestInt(), juce::Justification::centred);
}

juce::String FreOscEditor::OctaveSelector::getOctaveText(OctaveValue octave) const
{
    switch (octave)
    {
        case Minus2: return "-2";
        case Minus1: return "-1";
        case Zero: return "0";
        case Plus1: return "+1";
        case Plus2: return "+2";
        default: return "0";
    }
}

void FreOscEditor::OctaveSelector::mouseDown(const juce::MouseEvent& event)
{
    auto pos = event.getPosition();
    
    OctaveValue newOctave = selectedOctave; // Keep current if no valid click
    bool foundValidClick = false;
    
    // Check each button area more precisely - ensure complete coverage
    if (minus2Button.contains(pos))
    {
        newOctave = Minus2;
        foundValidClick = true;
    }
    else if (minus1Button.contains(pos))
    {
        newOctave = Minus1;
        foundValidClick = true;
    }
    else if (zeroButton.contains(pos))
    {
        newOctave = Zero;
        foundValidClick = true;
    }
    else if (plus1Button.contains(pos))
    {
        newOctave = Plus1;
        foundValidClick = true;
    }
    else if (plus2Button.contains(pos))
    {
        newOctave = Plus2;
        foundValidClick = true;
    }
    
    // Only change selection if we clicked on a valid button AND it's different
    // This prevents deselection and ensures radio button behavior (always one selected)
    if (foundValidClick && newOctave != selectedOctave)
    {
        selectedOctave = newOctave;
        repaint();
        
        if (onOctaveChanged)
            onOctaveChanged(static_cast<int>(selectedOctave));
    }
}

void FreOscEditor::OctaveSelector::mouseMove(const juce::MouseEvent& event)
{
    auto pos = event.getPosition();
    OctaveValue newHover = Zero;
    bool wasHovering = isHovering;
    
    if (minus2Button.contains(pos))
    {
        newHover = Minus2;
        isHovering = true;
    }
    else if (minus1Button.contains(pos))
    {
        newHover = Minus1;
        isHovering = true;
    }
    else if (zeroButton.contains(pos))
    {
        newHover = Zero;
        isHovering = true;
    }
    else if (plus1Button.contains(pos))
    {
        newHover = Plus1;
        isHovering = true;
    }
    else if (plus2Button.contains(pos))
    {
        newHover = Plus2;
        isHovering = true;
    }
    else
    {
        isHovering = false;
    }
    
    if (isHovering != wasHovering || (isHovering && newHover != hoveredOctave))
    {
        hoveredOctave = newHover;
        repaint();
    }
}

void FreOscEditor::OctaveSelector::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    if (isHovering)
    {
        isHovering = false;
        repaint();
    }
}

void FreOscEditor::OctaveSelector::resized()
{
    auto bounds = getLocalBounds().reduced(1); // Minimal outer padding for the border
    auto buttonWidth = bounds.getWidth() / 5; // 5 buttons: -2, -1, 0, +1, +2
    
    // Create buttons with no gaps between them for reliable hit detection
    minus2Button = juce::Rectangle<int>(bounds.getX(), bounds.getY(), buttonWidth, bounds.getHeight());
    minus1Button = juce::Rectangle<int>(bounds.getX() + buttonWidth, bounds.getY(), buttonWidth, bounds.getHeight());
    zeroButton = juce::Rectangle<int>(bounds.getX() + buttonWidth * 2, bounds.getY(), buttonWidth, bounds.getHeight());
    plus1Button = juce::Rectangle<int>(bounds.getX() + buttonWidth * 3, bounds.getY(), buttonWidth, bounds.getHeight());
    plus2Button = juce::Rectangle<int>(bounds.getX() + buttonWidth * 4, bounds.getY(), bounds.getWidth() - buttonWidth * 4, bounds.getHeight()); // Remaining width
}

void FreOscEditor::OctaveSelector::setSelectedOctave(OctaveValue octave)
{
    if (selectedOctave != octave)
    {
        selectedOctave = octave;
        repaint();
    }
}

juce::Rectangle<int> FreOscEditor::OctaveSelector::getButtonForOctave(OctaveValue octave) const
{
    switch (octave)
    {
        case Minus2: return minus2Button;
        case Minus1: return minus1Button;
        case Zero: return zeroButton;
        case Plus1: return plus1Button;
        case Plus2: return plus2Button;
        default: return {};
    }
}

//==============================================================================
// OctaveSelectorAttachment Implementation

FreOscEditor::OctaveSelectorAttachment::OctaveSelectorAttachment(
    juce::AudioProcessorValueTreeState& state, 
    const juce::String& parameterID, 
    OctaveSelector& selector)
    : valueTreeState(state), paramID(parameterID), octaveSelector(selector)
{
    // Add listener to parameter changes
    valueTreeState.addParameterListener(paramID, this);
    
    // Set initial value from parameter
    // For AudioParameterInt, we need to get the actual integer value
    if (auto* param = valueTreeState.getParameter(paramID))
    {
        // Try to cast to AudioParameterInt to get the proper value
        if (auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param))
        {
            auto octaveValue = intParam->get();  // This should be -2, -1, 0, 1, or 2
            // Convert from parameter range (-2 to 2) to enum (0 to 4)
            auto octave = octaveValue + 2; // -2 becomes 0, -1 becomes 1, 0 becomes 2, etc.
            octaveSelector.setSelectedOctave(octave);
        }
    }
    
    // Listen for selector changes
    octaveSelector.onOctaveChanged = [this](int octave) { selectorChanged(octave); };
}

FreOscEditor::OctaveSelectorAttachment::~OctaveSelectorAttachment()
{
    valueTreeState.removeParameterListener(paramID, this);
    octaveSelector.onOctaveChanged = nullptr;
}

void FreOscEditor::OctaveSelectorAttachment::parameterChanged(const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused(newValue); // We get the value directly from the parameter instead
    
    if (parameterID == paramID)
    {
        // Get the actual integer parameter directly
        if (auto* param = valueTreeState.getParameter(paramID))
        {
            if (auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param))
            {
                auto octaveValue = intParam->get(); // This should be -2, -1, 0, 1, or 2
                auto octave = octaveValue + 2; // Convert from -2..2 range to 0..4 enum
                octaveSelector.setSelectedOctave(octave);
            }
        }
    }
}

void FreOscEditor::OctaveSelectorAttachment::selectorChanged(int octave)
{
    if (auto* param = valueTreeState.getParameter(paramID))
    {
        // For AudioParameterInt, convert from enum (0-4) back to parameter range (-2 to 2)
        if (auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param))
        {
            auto octaveValue = octave - 2; // Convert from 0..4 enum to -2..2 range
            *intParam = octaveValue; // This uses the assignment operator which properly sets the value
        }
    }
}

//==============================================================================
FreOscEditor::FreOscEditor (FreOscProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), tabbedComponent(juce::TabbedButtonBar::TabsAtTop)
{
    // Set editor size - taller for horizontal layout
    setSize(1200, 800);

    // Make the editor resizable with constraints
    setResizable(true, true);

    // Set minimum and maximum size constraints - taller
    minUsableWidth = 800;   // Same minimum width
    minUsableHeight = 650;  // Increased minimum height

    getConstrainer()->setMinimumSize(400, 350);   // Absolute minimum (will scroll)
    getConstrainer()->setMaximumSize(1800, 1200); // 150% maximum

    // Maintain fixed aspect ratio based on initial size (1200x800)
    getConstrainer()->setFixedAspectRatio(1200.0 / 800.0);

    // Setup scrollable viewport system
    viewport.setViewedComponent(&contentComponent, false);
    viewport.setScrollBarsShown(true, true);
    addAndMakeVisible(viewport);

    // Setup header section (goes in content component)
    setupComponent(titleLabel);
    titleLabel.setText("FreOSC", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions().withHeight(24.0f).withStyle("Bold")));  // Fixed larger size, bold
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    setupComponent(presetSelector);
    presetSelector.addListener(this); // Auto-load presets on selection
    
    // Setup master volume components for header
    setupSlider(masterVolumeSlider, masterVolumeLabel, masterVolumeValue, "Master", "master_volume");
    masterVolumeSlider.setName("master_volume_knob"); // Identify for custom LED size
    
    // Style the master volume value label
    applyComponentStyling(masterVolumeValue);
    masterVolumeValue.setFont(juce::Font(juce::FontOptions().withHeight(12.0f))); // Slightly larger font
    masterVolumeValue.setJustificationType(juce::Justification::centredLeft); // Left-aligned
    masterVolumeValue.setColour(juce::Label::textColourId, juce::Colours::white);
    masterVolumeValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    // Add header components to content component instead of main editor
    contentComponent.addAndMakeVisible(titleLabel);
    contentComponent.addAndMakeVisible(presetSelector);
    
    // Add master volume components to header (no label, but add value label to the right)
    // contentComponent.addAndMakeVisible(masterVolumeLabel); - removed, no label shown
    contentComponent.addAndMakeVisible(masterVolumeSlider);
    contentComponent.addAndMakeVisible(masterVolumeValue); // Show value to the right of knob

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
        // LookAndFeel cleanup handled by main editor
        
        setLookAndFeel(nullptr);  // Remove from main editor
        tabbedComponent.getTabbedButtonBar().setLookAndFeel(nullptr);
    }
    stopTimer();
}

//==============================================================================
void FreOscEditor::paint (juce::Graphics& g)
{
    // Solid dark background - matching tab page background
    auto bounds = getLocalBounds();
    
    // Use same color as tab pages for consistency
    g.setColour(juce::Colour(0xff1a1a1a)); // Dark background matching tab pages
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

    // Calculate scaling factors based on content size vs original size (1200x800)
    float scaleX = contentWidth / 1200.0f;
    float scaleY = contentHeight / 800.0f;
    float scale = juce::jmin(scaleX, scaleY); // Use minimum to maintain proportions

    // Store scale for use in other methods
    currentScale = scale;

    // Update fonts for scaling
    updateScaledFonts();

    // Layout components in content component
    auto contentBounds = contentComponent.getLocalBounds();

    // Header section - taller to accommodate larger master volume knob
    auto headerBounds = contentBounds.removeFromTop(70);
    
    // Reserve more space for master volume first (right side) - wider area for knob + value label
    auto masterArea = headerBounds.removeFromRight(160).reduced(5);
    
    // Layout remaining header components with fixed title size and proper preset selector height
    titleLabel.setBounds(headerBounds.removeFromLeft(200).reduced(5)); // Smaller fixed width for title
    
    // Preset selector with proper height (center it vertically in the header)
    auto presetBounds = headerBounds.reduced(5);
    auto presetHeight = 25; // Fixed height for dropdown
    auto presetY = presetBounds.getY() + (presetBounds.getHeight() - presetHeight) / 2; // Center vertically
    presetSelector.setBounds(presetBounds.getX(), presetY, presetBounds.getWidth(), presetHeight);
    
    // Layout master volume knob in reserved area - knob on left, value on right
    // masterVolumeLabel.setBounds() - removed, no title
    
    // Reserve space for value label on the right (40px wide)
    auto valueArea = masterArea.removeFromRight(50);
    auto knobArea = masterArea; // Remaining space for knob
    
    // Make knob use available square space
    auto maxKnobSize = juce::jmin(knobArea.getWidth() - 2, knobArea.getHeight() - 2);
    auto knobBounds = knobArea.withSizeKeepingCentre(maxKnobSize, maxKnobSize);
    masterVolumeSlider.setBounds(knobBounds);
    
    // Position value label to the right of the knob, vertically centered
    masterVolumeValue.setBounds(valueArea);

    // Main area for tabs - reduced padding
    auto mainArea = contentBounds.reduced(5);
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
    // No buttons currently handled - load preset button removed
    juce::ignoreUnused(button);
}

void FreOscEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &presetSelector)
    {
        // Auto-load preset when selected from dropdown
        int selectedIndex = presetSelector.getSelectedItemIndex();
        if (selectedIndex > 0) // Index 0 is "Custom", start from 1
        {
            // Load the preset (selectedIndex - 1 because index 0 is "Custom", so preset 0 is at index 1)
            audioProcessor.loadPreset(selectedIndex - 1);
        }
    }
    // Rate controls are now always visible - no mode-based hiding
}

//==============================================================================
// Helper method implementations
void FreOscEditor::setupComponent(juce::Component& component)
{
    addAndMakeVisible(component);

    // Apply FL Studio 3x Osc inspired styling - metallic grey look
    if (auto* label = dynamic_cast<juce::Label*>(&component))
    {
        label->setColour(juce::Label::textColourId, juce::Colours::white); // Changed to white
        label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label->setJustificationType(juce::Justification::centred);
    }
    else if (auto* slider = dynamic_cast<juce::Slider*>(&component))
    {
        // Check if this is a vertical slider by checking its name
        juce::String sliderName = slider->getName();
        bool isVerticalSlider = sliderName.contains("level") || sliderName.contains("detune") || sliderName.contains("pan") || sliderName.contains("filter") || sliderName.contains("lfo");
        
        if (isVerticalSlider)
        {
            slider->setSliderStyle(juce::Slider::LinearVertical);
            slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            // Vertical slider styling to match knob design
            slider->setColour(juce::Slider::trackColourId, juce::Colour(0xff202020)); // Dark track like knob rim
            slider->setColour(juce::Slider::thumbColourId, juce::Colour(0xffc0c0c0)); // Metallic thumb like knob center
            slider->setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
        }
        else
        {
            slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            // Metallic knob styling like FL Studio
            slider->setColour(juce::Slider::rotarySliderFillColourId, accentColour);
            slider->setColour(juce::Slider::rotarySliderOutlineColourId, knobColour.darker(0.3f));
            slider->setColour(juce::Slider::thumbColourId, juce::Colour(0xff87ceeb)); // Light blue thumb
            slider->setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
        }
    }
    else if (auto* combo = dynamic_cast<juce::ComboBox*>(&component))
    {
        // Retro 80s black plastic dropdown styling (matching waveform/octave selectors)
        combo->setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a)); // Dark black plastic background
        combo->setColour(juce::ComboBox::textColourId, juce::Colour(0xffa0a0a0)); // Light grey text (unselected state)
        combo->setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff707070)); // Medium grey outline (raised look)
        combo->setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffa0a0a0)); // Light grey arrow
        combo->setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff2a2a2a)); // Same as background
        combo->setColour(juce::ComboBox::focusedOutlineColourId, juce::Colour(0xff909090)); // Slightly brighter when focused
        
        // Dropdown menu colors (popup window)
        combo->setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff2a2a2a)); // Dark popup background
        combo->setColour(juce::PopupMenu::textColourId, juce::Colour(0xffa0a0a0)); // Light grey text in popup
        combo->setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff404040)); // Highlighted item background
        combo->setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white); // White text when highlighted
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
        // GroupComponent colors now handled by LookAndFeel - just set text color
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

    // Waveform and octave selectors are custom components - no combo box setup needed
    section.waveformLabel.setText("Waveform", juce::dontSendNotification);
    section.octaveLabel.setText("Octave", juce::dontSendNotification);
    setupSlider(section.levelSlider, section.levelLabel, section.levelValue, "Level", paramPrefix + "level");
    setupSlider(section.detuneSlider, section.detuneLabel, section.detuneValue, "Detune", paramPrefix + "detune");
    setupSlider(section.panSlider, section.panLabel, section.panValue, "Pan", paramPrefix + "pan");
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
    section.waveformSelector.setBounds(waveformArea);

    auto octaveArea = topRow.reduced(2);
    section.octaveLabel.setBounds(octaveArea.removeFromTop(12));
    section.octaveSelector.setBounds(octaveArea);

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

// layoutFilterSection() removed - now using GroupComponent layouts in tabs

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

void FreOscEditor::layoutPMSection()
{
    auto bounds = pmGroup.getBounds().reduced(10);
    auto rowHeight = bounds.getHeight() / 4; // Updated to 4 rows since we removed source

    // Amount
    auto row = bounds.removeFromTop(rowHeight);
    pmIndexLabel.setBounds(row.removeFromTop(15));
    pmIndexSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    pmIndexValue.setBounds(row);

    // Ratio
    row = bounds.removeFromTop(rowHeight);
    pmRatioLabel.setBounds(row.removeFromTop(15));
    pmRatioSlider.setBounds(row.removeFromTop(row.getHeight() - 15));
    pmRatioValue.setBounds(row);

    // Carrier (Message signal is always Oscillator 3)
    row = bounds.removeFromTop(rowHeight);
    pmCarrierLabel.setBounds(row.removeFromLeft(60));
    pmCarrierCombo.setBounds(row);
}




void FreOscEditor::layoutLFOSection()
{
    auto bounds = lfoGroup.getBounds().reduced(10);
    auto rowHeight = bounds.getHeight() / 5;

    // Waveform
    auto row = bounds.removeFromTop(rowHeight);
    lfoWaveformLabel.setBounds(row.removeFromLeft(60));
    lfoWaveformSelector.setBounds(row);

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
    // Waveform and octave selectors don't need combo box setup - they handle their own visuals

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

    // Filter routing options
    filterRoutingCombo.addItem("Filter 1 Only", 1);
    filterRoutingCombo.addItem("Parallel", 2);
    filterRoutingCombo.addItem("Series", 3);

    // Effects routing options
    effectsRoutingCombo.addItem("Wavefolder to Reverb to Delay", 1);
    effectsRoutingCombo.addItem("Wavefolder to Delay to Reverb", 2);
    effectsRoutingCombo.addItem("Wavefolder Parallel with Reverb+Delay", 3);

    // Filter 1 type options
    filterTypeCombo.addItem("Low Pass", 1);
    filterTypeCombo.addItem("High Pass", 2);
    filterTypeCombo.addItem("Band Pass", 3);

    // Filter 2 type options (same as Filter 1)
    filter2TypeCombo.addItem("Low Pass", 1);
    filter2TypeCombo.addItem("High Pass", 2);
    filter2TypeCombo.addItem("Band Pass", 3);


    // FM source is always Oscillator 3 - no combo box needed

    // FM target options - updated for new routing
    pmCarrierCombo.addItem("Oscillator 1", 1);
    pmCarrierCombo.addItem("Oscillator 2", 2);
    pmCarrierCombo.addItem("Both Osc 1 & 2", 3);

    // LFO waveform selector uses custom component - no setup needed

    // LFO target options
    lfoTargetCombo.addItem("None", 1);
    lfoTargetCombo.addItem("Pitch", 2);
    lfoTargetCombo.addItem("Filter Cutoff", 3);
    lfoTargetCombo.addItem("Filter2 Cutoff", 4);
    lfoTargetCombo.addItem("Volume", 5);
    lfoTargetCombo.addItem("Pan", 6);

    // Modulation Envelope target options (use addItemList for proper 0-based indexing)
    juce::StringArray modEnvTargetOptions = {"None", "PM Index", "PM Ratio", "Filter Cutoff", "Filter2 Cutoff"};
    modEnv1TargetCombo.addItemList(modEnvTargetOptions, 1);  // Base ID 1 for 0-based indexing
    modEnv2TargetCombo.addItemList(modEnvTargetOptions, 1);
    
    // Modulation Envelope mode options (use addItemList for proper 0-based indexing)
    juce::StringArray envelopeModeOptions = {"One-Shot", "Gate", "Looping"};
    modEnv1ModeCombo.addItemList(envelopeModeOptions, 1);  // Base ID 1 for 0-based indexing
    modEnv2ModeCombo.addItemList(envelopeModeOptions, 1);

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

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "filter2_cutoff", cutoff2Slider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "filter2_resonance", resonance2Slider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "filter2_gain", filterGain2Slider));

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "pm_index", pmIndexSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "pm_ratio", pmRatioSlider));

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "lfo_rate", lfoRateSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "lfo_amount", lfoAmountSlider));

    // Modulation Envelope 1 attachments
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "mod_env1_attack", modEnv1AttackSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "mod_env1_decay", modEnv1DecaySlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "mod_env1_sustain", modEnv1SustainSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "mod_env1_release", modEnv1ReleaseSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "mod_env1_amount", modEnv1AmountSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "mod_env1_rate", modEnv1RateSlider));

    // Modulation Envelope 2 attachments
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "mod_env2_attack", modEnv2AttackSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "mod_env2_decay", modEnv2DecaySlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "mod_env2_sustain", modEnv2SustainSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "mod_env2_release", modEnv2ReleaseSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "mod_env2_amount", modEnv2AmountSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "mod_env2_rate", modEnv2RateSlider));

    // Dynamics parameters removed - now uses fixed internal settings

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "plate_predelay", platePreDelaySlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "plate_size", plateSizeSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "plate_damping", plateDampingSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "plate_diffusion", plateDiffusionSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "plate_wet_level", plateWetSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "plate_width", plateWidthSlider));

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "tape_time", tapeTimeSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "tape_feedback", tapeFeedbackSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "tape_tone", tapeToneSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "tape_flutter", tapeFlutterSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "tape_wet_level", tapeWetSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "tape_width", tapeWidthSlider));

    // Wavefolder parameters
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "wavefolder_drive", wavefolderDriveSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "wavefolder_threshold", wavefolderThresholdSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "wavefolder_symmetry", wavefolderSymmetrySlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "wavefolder_mix", wavefolderMixSlider));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "wavefolder_output", wavefolderOutputSlider));

    // Create waveform selector attachments
    waveformAttachments.push_back(std::make_unique<WaveformSelectorAttachment>(valueTreeState, "osc1_waveform", osc1Section.waveformSelector));
    waveformAttachments.push_back(std::make_unique<WaveformSelectorAttachment>(valueTreeState, "osc2_waveform", osc2Section.waveformSelector));
    waveformAttachments.push_back(std::make_unique<WaveformSelectorAttachment>(valueTreeState, "osc3_waveform", osc3Section.waveformSelector));

    // Create octave selector attachments
    octaveAttachments.push_back(std::make_unique<OctaveSelectorAttachment>(valueTreeState, "osc1_octave", osc1Section.octaveSelector));
    octaveAttachments.push_back(std::make_unique<OctaveSelectorAttachment>(valueTreeState, "osc2_octave", osc2Section.octaveSelector));
    octaveAttachments.push_back(std::make_unique<OctaveSelectorAttachment>(valueTreeState, "osc3_octave", osc3Section.octaveSelector));

    // Create combo box attachments (for remaining dropdowns)

    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "noise_type", noiseTypeCombo));

    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "filter_routing", filterRoutingCombo));
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "filter_type", filterTypeCombo));
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "filter2_type", filter2TypeCombo));

    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "effects_routing", effectsRoutingCombo));

    // FM source is always Osc3 - no parameter attachment needed
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "pm_carrier", pmCarrierCombo));

    lfoWaveformAttachments.push_back(std::make_unique<LFOWaveformSelectorAttachment>(valueTreeState, "lfo_waveform", lfoWaveformSelector));
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "lfo_target", lfoTargetCombo));

    // Modulation Envelope target attachments
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "mod_env1_target", modEnv1TargetCombo));
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "mod_env2_target", modEnv2TargetCombo));
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "mod_env1_mode", modEnv1ModeCombo));
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(valueTreeState, "mod_env2_mode", modEnv2ModeCombo));
}

void FreOscEditor::updateValueLabels()
{
    // Update oscillator value labels
    osc1Section.levelValue.setText(juce::String(static_cast<int>(osc1Section.levelSlider.getValue() * 100)) + "%", juce::dontSendNotification);
    osc1Section.detuneValue.setText(juce::String(static_cast<int>(osc1Section.detuneSlider.getValue())) + "¢", juce::dontSendNotification);
    osc1Section.panValue.setText(formatPanValue(static_cast<float>(osc1Section.panSlider.getValue())), juce::dontSendNotification);

    osc2Section.levelValue.setText(juce::String(static_cast<int>(osc2Section.levelSlider.getValue() * 100)) + "%", juce::dontSendNotification);
    osc2Section.detuneValue.setText(juce::String(static_cast<int>(osc2Section.detuneSlider.getValue())) + "¢", juce::dontSendNotification);
    osc2Section.panValue.setText(formatPanValue(static_cast<float>(osc2Section.panSlider.getValue())), juce::dontSendNotification);

    osc3Section.levelValue.setText(juce::String(static_cast<int>(osc3Section.levelSlider.getValue() * 100)) + "%", juce::dontSendNotification);
    osc3Section.detuneValue.setText(juce::String(static_cast<int>(osc3Section.detuneSlider.getValue())) + "¢", juce::dontSendNotification);
    osc3Section.panValue.setText(formatPanValue(static_cast<float>(osc3Section.panSlider.getValue())), juce::dontSendNotification);

    // Update other value labels
    noiseLevelValue.setText(juce::String(static_cast<int>(noiseLevelSlider.getValue() * 100)) + "%", juce::dontSendNotification);
    noisePanValue.setText(formatPanValue(static_cast<float>(noisePanSlider.getValue())), juce::dontSendNotification);

    // Update master volume tooltip and value label - show dB value
    auto masterVolumeDbText = formatMasterVolumeValue(static_cast<float>(masterVolumeSlider.getValue()));
    masterVolumeSlider.setTooltip("Master Volume: " + masterVolumeDbText);
    masterVolumeValue.setText(masterVolumeDbText, juce::dontSendNotification);

    attackValue.setText(formatTimeValue(static_cast<float>(attackSlider.getValue())), juce::dontSendNotification);
    decayValue.setText(formatTimeValue(static_cast<float>(decaySlider.getValue())), juce::dontSendNotification);
    sustainValue.setText(juce::String(static_cast<int>(sustainSlider.getValue() * 100)) + "%", juce::dontSendNotification);
    releaseValue.setText(formatTimeValue(static_cast<float>(releaseSlider.getValue())), juce::dontSendNotification);

    cutoffValue.setText(formatFrequencyValue(static_cast<float>(cutoffSlider.getValue())), juce::dontSendNotification);
    resonanceValue.setText(formatResonanceValue(static_cast<float>(resonanceSlider.getValue())), juce::dontSendNotification);
    filterGainValue.setText(formatFilterGainValue(static_cast<float>(filterGainSlider.getValue())), juce::dontSendNotification);

    // Update Filter 2 value labels
    cutoff2Value.setText(formatFrequencyValue(static_cast<float>(cutoff2Slider.getValue())), juce::dontSendNotification);
    resonance2Value.setText(formatResonanceValue(static_cast<float>(resonance2Slider.getValue())), juce::dontSendNotification);
    filterGain2Value.setText(formatFilterGainValue(static_cast<float>(filterGain2Slider.getValue())), juce::dontSendNotification);

    pmIndexValue.setText(juce::String(pmIndexSlider.getValue(), 1), juce::dontSendNotification);
    pmRatioValue.setText(formatPMRatioValue(static_cast<float>(pmRatioSlider.getValue())), juce::dontSendNotification);

    // Dynamics removed from user control - values updated automatically with fixed settings

    platePreDelayValue.setText(juce::String(static_cast<int>(platePreDelaySlider.getValue() * 250.0f)) + "ms", juce::dontSendNotification);
    plateSizeValue.setText(juce::String(static_cast<int>(plateSizeSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);
    plateDampingValue.setText(juce::String(static_cast<int>(plateDampingSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);
    plateDiffusionValue.setText(juce::String(static_cast<int>(plateDiffusionSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);
    plateWetValue.setText(juce::String(static_cast<int>(plateWetSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);
    plateWidthValue.setText(juce::String(static_cast<int>(plateWidthSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);

    tapeTimeValue.setText(juce::String(static_cast<int>(tapeTimeSlider.getValue() * (2000.0f - 20.0f) + 20.0f)) + "ms", juce::dontSendNotification);
    tapeFeedbackValue.setText(juce::String(static_cast<int>(tapeFeedbackSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);
    tapeToneValue.setText(juce::String(static_cast<int>(tapeToneSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);
    tapeFlutterValue.setText(juce::String(static_cast<int>(tapeFlutterSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);
    tapeWetValue.setText(juce::String(static_cast<int>(tapeWetSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);
    tapeWidthValue.setText(juce::String(static_cast<int>(tapeWidthSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);

    // Wavefolder value labels
    wavefolderDriveValue.setText(juce::String(static_cast<int>(wavefolderDriveSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);
    wavefolderThresholdValue.setText(juce::String(static_cast<int>(wavefolderThresholdSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);
    wavefolderSymmetryValue.setText(juce::String(static_cast<int>(wavefolderSymmetrySlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);
    wavefolderMixValue.setText(juce::String(static_cast<int>(wavefolderMixSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);
    wavefolderOutputValue.setText(juce::String(static_cast<int>(wavefolderOutputSlider.getValue() * 100.0f)) + "%", juce::dontSendNotification);

    lfoRateValue.setText(juce::String(lfoRateSlider.getValue(), 2) + " Hz", juce::dontSendNotification);
    lfoAmountValue.setText(juce::String(static_cast<int>(lfoAmountSlider.getValue() * 100)) + "%", juce::dontSendNotification);

    // Modulation Envelope 1 value labels
    modEnv1AttackValue.setText(formatTimeValue(static_cast<float>(modEnv1AttackSlider.getValue())), juce::dontSendNotification);
    modEnv1DecayValue.setText(formatTimeValue(static_cast<float>(modEnv1DecaySlider.getValue())), juce::dontSendNotification);
    modEnv1SustainValue.setText(juce::String(static_cast<int>(modEnv1SustainSlider.getValue() * 100)) + "%", juce::dontSendNotification);
    modEnv1ReleaseValue.setText(formatTimeValue(static_cast<float>(modEnv1ReleaseSlider.getValue())), juce::dontSendNotification);
    modEnv1AmountValue.setText(juce::String(static_cast<int>(modEnv1AmountSlider.getValue() * 100)) + "%", juce::dontSendNotification);
    modEnv1RateValue.setText(juce::String(modEnv1RateSlider.getValue(), 1) + " Hz", juce::dontSendNotification);

    // Modulation Envelope 2 value labels
    modEnv2AttackValue.setText(formatTimeValue(static_cast<float>(modEnv2AttackSlider.getValue())), juce::dontSendNotification);
    modEnv2DecayValue.setText(formatTimeValue(static_cast<float>(modEnv2DecaySlider.getValue())), juce::dontSendNotification);
    modEnv2SustainValue.setText(juce::String(static_cast<int>(modEnv2SustainSlider.getValue() * 100)) + "%", juce::dontSendNotification);
    modEnv2ReleaseValue.setText(formatTimeValue(static_cast<float>(modEnv2ReleaseSlider.getValue())), juce::dontSendNotification);
    modEnv2AmountValue.setText(juce::String(static_cast<int>(modEnv2AmountSlider.getValue() * 100)) + "%", juce::dontSendNotification);
    modEnv2RateValue.setText(juce::String(modEnv2RateSlider.getValue(), 1) + " Hz", juce::dontSendNotification);
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
    // Convert normalized value (0.0-1.0) to Q (0.1-5.0)
    // This matches FreOscFilter::normalizedToQ()
    float q = 0.1f + (juce::jlimit(0.0f, 1.0f, normalizedValue) * 4.9f);
    return juce::String(q, 1) + "Q";
}

juce::String FreOscEditor::formatFilterGainValue(float normalizedValue)
{
    // Convert normalized value (0.0-1.0) to dB (-24dB to +24dB)
    // This matches FreOscFilter::normalizedToGainDb()
    float gainDb = -24.0f + (juce::jlimit(0.0f, 1.0f, normalizedValue) * 48.0f);
    return juce::String(gainDb, 1) + "dB";
}

juce::String FreOscEditor::formatPMRatioValue(float ratioValue)
{
    // Convert decimal ratio to fraction format (e.g., 1.5 -> "3:2", 2.0 -> "2:1")
    
    // Handle simple integer ratios first
    if (std::abs(ratioValue - std::round(ratioValue)) < 0.01f)
    {
        int intRatio = static_cast<int>(std::round(ratioValue));
        return juce::String(intRatio) + ":1";
    }
    
    // For fractional ratios, find best rational approximation
    // Common musical ratios in FM synthesis
    struct RatioPair { float decimal; juce::String display; };
    static const RatioPair commonRatios[] = {
        {0.5f, "1:2"}, {0.667f, "2:3"}, {0.75f, "3:4"}, {1.0f, "1:1"},
        {1.25f, "5:4"}, {1.33f, "4:3"}, {1.5f, "3:2"}, {1.67f, "5:3"},
        {2.0f, "2:1"}, {2.5f, "5:2"}, {3.0f, "3:1"}, {4.0f, "4:1"},
        {5.0f, "5:1"}, {6.0f, "6:1"}, {7.0f, "7:1"}, {8.0f, "8:1"}
    };
    
    // Find closest match
    float bestError = 1000.0f;
    juce::String bestRatio = juce::String(ratioValue, 1) + ":1";
    
    for (const auto& ratio : commonRatios)
    {
        float error = std::abs(ratioValue - ratio.decimal);
        if (error < bestError)
        {
            bestError = error;
            bestRatio = ratio.display;
        }
    }
    
    // If no close match found (error > 0.05), show decimal format
    if (bestError > 0.05f)
    {
        return juce::String(ratioValue, 1) + ":1";
    }
    
    return bestRatio;
}

juce::String FreOscEditor::formatMasterVolumeValue(float normalizedValue)
{
    // Convert normalized master volume to dB display
    // Matches the conversion in FreOscProcessor::normalizedToMasterGain()
    normalizedValue = juce::jlimit(0.0f, 1.0f, normalizedValue);
    
    if (normalizedValue <= 0.0f)
        return "-inf dB";
    
    if (normalizedValue <= 0.75f)
    {
        // Map 0.0-0.75 to -60dB to 0dB  
        float normalizedAttenuation = normalizedValue / 0.75f;
        float dbValue = -60.0f + (normalizedAttenuation * 60.0f);
        return juce::String(dbValue, 1) + "dB";
    }
    else
    {
        // Map 0.75-1.0 to 0dB to +24dB
        float normalizedBoost = (normalizedValue - 0.75f) / 0.25f;
        float dbValue = normalizedBoost * 24.0f;
        return "+" + juce::String(dbValue, 1) + "dB";
    }
}

void FreOscEditor::updateScaledFonts()
{
    // Keep title font fixed size - don't scale it
    titleLabel.setFont(juce::Font(juce::FontOptions().withHeight(24.0f).withStyle("Bold")));

    // Update all label fonts throughout the interface
    // Note: In a more complex implementation, we would iterate through all labels
    // For now, fonts will update as components are laid out
}

//==============================================================================
// Tab Implementation Methods

void FreOscEditor::createTabbedInterface()
{
    // Setup tabbed component with GroupComponent matching theme
    tabbedComponent.setTabBarDepth(28);
    tabbedComponent.setOutline(1);
    tabbedComponent.setColour(juce::TabbedComponent::backgroundColourId, juce::Colour(0xff1a1a1a)); // Match GroupComponent background
    tabbedComponent.setColour(juce::TabbedComponent::outlineColourId, juce::Colour(0xff1a1a1a)); // Match GroupComponent background for border

    // Apply custom look and feel to remove gradients AND style knobs
    auto& tabBar = tabbedComponent.getTabbedButtonBar();
    tabBar.setLookAndFeel(customLookAndFeel.get());
    
    // Apply custom look and feel to the entire editor to style all knobs
    setLookAndFeel(customLookAndFeel.get());

    // Create tab content (removed master tab)
    oscillatorsTab = createOscillatorsTab();
    filterEnvelopeTab = createFilterEnvelopeTab();
    modulationTab = createModulationTab();
    effectsTab = createEffectsTab();
    
    // All GroupComponent colors now handled by LookAndFeel

    // Add tabs with GroupComponent matching theme
    const auto groupComponentBg = juce::Colour(0xff1a1a1a); // Match GroupComponent background
    tabbedComponent.addTab("OSC", groupComponentBg, oscillatorsTab.get(), false);
    tabbedComponent.addTab("FILTER", groupComponentBg, filterEnvelopeTab.get(), false);
    tabbedComponent.addTab("MOD", groupComponentBg, modulationTab.get(), false);
    tabbedComponent.addTab("EFFECTS", groupComponentBg, effectsTab.get(), false);

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
            // Setup all oscillators with GroupComponent
            setupGroupOscillator(owner.osc1Section, "Oscillator 1");
            setupGroupOscillator(owner.osc2Section, "Oscillator 2");
            setupGroupOscillator(owner.osc3Section, "Oscillator 3");

            // Setup noise generator and envelope with GroupComponent (side by side)
            setupGroupNoise();
            setupGroupEnvelope();
        }

        void setupGroupOscillator(FreOscEditor::OscillatorSection& section, const juce::String& title)
        {
            // Setup oscillator using GroupComponent (for osc1)
            owner.applyComponentStyling(section.group);
            section.group.setText(title);
            
            // Setup and add all components to the group
            // Note: waveformSelector and octaveSelector don't need applyComponentStyling as they're custom
            
            // Set names for oscillator sliders so they can be identified for vertical styling
            section.levelSlider.setName(title + "_level");
            section.detuneSlider.setName(title + "_detune"); 
            section.panSlider.setName(title + "_pan");
            
            owner.applyComponentStyling(section.levelSlider);
            owner.applyComponentStyling(section.detuneSlider);
            owner.applyComponentStyling(section.panSlider);
            
            owner.applyComponentStyling(section.waveformLabel);
            owner.applyComponentStyling(section.octaveLabel);
            owner.applyComponentStyling(section.levelLabel);
            owner.applyComponentStyling(section.detuneLabel);
            owner.applyComponentStyling(section.panLabel);
            
            owner.applyComponentStyling(section.levelValue);
            owner.applyComponentStyling(section.detuneValue);
            owner.applyComponentStyling(section.panValue);

            // Set value label styling with white text
            section.levelValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            section.levelValue.setJustificationType(juce::Justification::centred);
            section.levelValue.setColour(juce::Label::textColourId, juce::Colours::white);
            section.levelValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            section.detuneValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            section.detuneValue.setJustificationType(juce::Justification::centred);
            section.detuneValue.setColour(juce::Label::textColourId, juce::Colours::white);
            section.detuneValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            section.panValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            section.panValue.setJustificationType(juce::Justification::centred);
            section.panValue.setColour(juce::Label::textColourId, juce::Colours::white);
            section.panValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // Set labels
            section.waveformLabel.setText("Waveform", juce::dontSendNotification);
            section.octaveLabel.setText("Octave", juce::dontSendNotification);
            section.levelLabel.setText("Level", juce::dontSendNotification);
            section.detuneLabel.setText("Detune", juce::dontSendNotification);
            section.panLabel.setText("Pan", juce::dontSendNotification);

            // Add ALL components to the group
            section.group.addAndMakeVisible(section.waveformLabel);
            section.group.addAndMakeVisible(section.waveformSelector);
            section.group.addAndMakeVisible(section.octaveLabel);
            section.group.addAndMakeVisible(section.octaveSelector);
            section.group.addAndMakeVisible(section.levelLabel);
            section.group.addAndMakeVisible(section.levelSlider);
            section.group.addAndMakeVisible(section.levelValue);
            section.group.addAndMakeVisible(section.detuneLabel);
            section.group.addAndMakeVisible(section.detuneSlider);
            section.group.addAndMakeVisible(section.detuneValue);
            section.group.addAndMakeVisible(section.panLabel);
            section.group.addAndMakeVisible(section.panSlider);
            section.group.addAndMakeVisible(section.panValue);

            // Add the group to the tab
            addAndMakeVisible(section.group);
        }

        void setupGroupNoise()
        {
            // Setup noise generator using GroupComponent
            owner.applyComponentStyling(owner.noiseGroup);
            owner.noiseGroup.setText("Noise Generator");
            
            // Setup and add all components to the group
            owner.applyComponentStyling(owner.noiseTypeCombo);
            
            // Set names for noise sliders so they can be identified as rotary knobs
            owner.noiseLevelSlider.setName("noise_level");
            owner.noisePanSlider.setName("noise_pan");
            
            owner.applyComponentStyling(owner.noiseLevelSlider);
            owner.applyComponentStyling(owner.noisePanSlider);
            
            // noiseTypeLabel removed - no label needed
            owner.applyComponentStyling(owner.noiseLevelLabel);
            owner.applyComponentStyling(owner.noisePanLabel);
            owner.applyComponentStyling(owner.noiseLevelValue);
            owner.applyComponentStyling(owner.noisePanValue);

            // Set value label styling for noise generator with white text
            owner.noiseLevelValue.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.noiseLevelValue.setJustificationType(juce::Justification::centred);
            owner.noiseLevelValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.noiseLevelValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.noisePanValue.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.noisePanValue.setJustificationType(juce::Justification::centred);
            owner.noisePanValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.noisePanValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // Set labels
            // Type label removed
            owner.noiseLevelLabel.setText("Level", juce::dontSendNotification);
            owner.noisePanLabel.setText("Pan", juce::dontSendNotification);

            // Add ALL components to the group
            // noiseTypeLabel removed
            owner.noiseGroup.addAndMakeVisible(owner.noiseTypeCombo);
            owner.noiseGroup.addAndMakeVisible(owner.noiseLevelLabel);
            owner.noiseGroup.addAndMakeVisible(owner.noiseLevelSlider);
            owner.noiseGroup.addAndMakeVisible(owner.noiseLevelValue);
            owner.noiseGroup.addAndMakeVisible(owner.noisePanLabel);
            owner.noiseGroup.addAndMakeVisible(owner.noisePanSlider);
            owner.noiseGroup.addAndMakeVisible(owner.noisePanValue);

            // Add the group to the tab
            addAndMakeVisible(owner.noiseGroup);
        }

        void setupGroupEnvelope()
        {
            // Setup envelope generator using GroupComponent (to match noise generator styling)
            owner.applyComponentStyling(owner.envelopeGroup);
            owner.envelopeGroup.setText("Envelope");
            
            // Setup and add all components to the group
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

            // Set value label styling for envelope generator with white text
            owner.attackValue.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.attackValue.setJustificationType(juce::Justification::centred);
            owner.attackValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.attackValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.decayValue.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.decayValue.setJustificationType(juce::Justification::centred);
            owner.decayValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.decayValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.sustainValue.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.sustainValue.setJustificationType(juce::Justification::centred);
            owner.sustainValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.sustainValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.releaseValue.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.releaseValue.setJustificationType(juce::Justification::centred);
            owner.releaseValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.releaseValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // Set labels
            owner.attackLabel.setText("Attack", juce::dontSendNotification);
            owner.decayLabel.setText("Decay", juce::dontSendNotification);
            owner.sustainLabel.setText("Sustain", juce::dontSendNotification);
            owner.releaseLabel.setText("Release", juce::dontSendNotification);

            // Add ALL components to the group
            owner.envelopeGroup.addAndMakeVisible(owner.attackLabel);
            owner.envelopeGroup.addAndMakeVisible(owner.attackSlider);
            owner.envelopeGroup.addAndMakeVisible(owner.attackValue);
            owner.envelopeGroup.addAndMakeVisible(owner.decayLabel);
            owner.envelopeGroup.addAndMakeVisible(owner.decaySlider);
            owner.envelopeGroup.addAndMakeVisible(owner.decayValue);
            owner.envelopeGroup.addAndMakeVisible(owner.sustainLabel);
            owner.envelopeGroup.addAndMakeVisible(owner.sustainSlider);
            owner.envelopeGroup.addAndMakeVisible(owner.sustainValue);
            owner.envelopeGroup.addAndMakeVisible(owner.releaseLabel);
            owner.envelopeGroup.addAndMakeVisible(owner.releaseSlider);
            owner.envelopeGroup.addAndMakeVisible(owner.releaseValue);

            // Add the group to the tab
            addAndMakeVisible(owner.envelopeGroup);
        }

        void setupDirectOscillator(FreOscEditor::OscillatorSection& section, [[maybe_unused]] const juce::String& title, [[maybe_unused]] const juce::String& paramPrefix)
        {
            // DIRECT SETUP: Add components directly to tab (no GroupComponent)
            // This avoids coordinate system conflicts

            // Setup components with proper styling
            // Note: waveformSelector doesn't need applyComponentStyling
            owner.applyComponentStyling(section.octaveSelector);
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
            section.waveformLabel.setText("Waveform", juce::dontSendNotification);
            section.octaveLabel.setText("Octave", juce::dontSendNotification);
            section.levelLabel.setText("Level", juce::dontSendNotification);
            section.detuneLabel.setText("Detune", juce::dontSendNotification);
            section.panLabel.setText("Pan", juce::dontSendNotification);

            // Add ALL components directly to the tab (not to a group) - INCLUDING VALUE LABELS
            addAndMakeVisible(section.waveformLabel);
            addAndMakeVisible(section.waveformSelector);
            addAndMakeVisible(section.octaveLabel);
            addAndMakeVisible(section.octaveSelector);
            addAndMakeVisible(section.levelLabel);
            addAndMakeVisible(section.levelSlider);
            addAndMakeVisible(section.levelValue);  // CRITICAL: Add value label to hierarchy
            addAndMakeVisible(section.detuneLabel);
            addAndMakeVisible(section.detuneSlider);
            addAndMakeVisible(section.detuneValue); // CRITICAL: Add value label to hierarchy
            addAndMakeVisible(section.panLabel);
            addAndMakeVisible(section.panSlider);
            addAndMakeVisible(section.panValue);    // CRITICAL: Add value label to hierarchy

            // Section setup complete
        }

        void setupDirectNoise()
        {
            // DIRECT SETUP: Add noise components directly to tab (no GroupComponent)

            // Setup components with proper styling
            owner.applyComponentStyling(owner.noiseTypeCombo);
            
            // Set names for noise sliders so they can be identified as rotary knobs
            owner.noiseLevelSlider.setName("noise_level");
            owner.noisePanSlider.setName("noise_pan");
            
            owner.applyComponentStyling(owner.noiseLevelSlider);
            owner.applyComponentStyling(owner.noisePanSlider);

            // noiseTypeLabel removed - no label needed
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
            // Type label removed
            owner.noiseLevelLabel.setText("Level", juce::dontSendNotification);
            owner.noisePanLabel.setText("Pan", juce::dontSendNotification);

            // Add ALL components directly to the tab (not to a group) - INCLUDING VALUE LABELS
            // noiseTypeLabel removed
            addAndMakeVisible(owner.noiseTypeCombo);
            addAndMakeVisible(owner.noiseLevelLabel);
            addAndMakeVisible(owner.noiseLevelSlider);
            addAndMakeVisible(owner.noiseLevelValue);  // Value label for level
            addAndMakeVisible(owner.noisePanLabel);
            addAndMakeVisible(owner.noisePanSlider);
            addAndMakeVisible(owner.noisePanValue);    // Value label for pan
        }

        void layoutGroupOscillator(FreOscEditor::OscillatorSection& section, juce::Rectangle<int> area)
        {
            // Layout oscillator using GroupComponent
            section.group.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = section.group.getLocalBounds().reduced(8); // Padding inside group
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight)); // Remove space for bottom band
            
            // Two-row layout: Dropdowns on top, knobs underneath
            auto topRowHeight = 40; // Reduced height for dropdown section
            auto topRow = bounds.removeFromTop(topRowHeight);
            auto bottomRow = bounds; // Remaining space for knobs
            
            // Top row: Waveform and Octave dropdowns side by side
            auto dropdownWidth = topRow.getWidth() / 2;
            
            // Waveform (left half) - now uses custom selector
            auto waveformArea = topRow.removeFromLeft(dropdownWidth).reduced(2);
            section.waveformLabel.setBounds(waveformArea.removeFromTop(15)); // Increased from 12 to 15 for padding
            section.waveformSelector.setBounds(waveformArea);

            // Octave (right half)
            auto octaveArea = topRow.reduced(2);
            section.octaveLabel.setBounds(octaveArea.removeFromTop(15)); // Increased from 12 to 15 for padding
            section.octaveSelector.setBounds(octaveArea);

            // Bottom row: Level, Detune, Pan knobs horizontally
            auto knobWidth = bottomRow.getWidth() / 3;
            
            const int minKnobSize = 55; // Slightly larger knobs
            const int minLabelHeight = 12; // Reduced label height
            const int minValueHeight = 12; // Reduced value height

            // Level knob
            auto levelArea = bottomRow.removeFromLeft(knobWidth).reduced(3);
            section.levelLabel.setBounds(levelArea.removeFromTop(minLabelHeight));
            auto levelValueArea = levelArea.removeFromBottom(minValueHeight);
            auto levelKnobArea = levelArea.reduced(0, 2); // Reduce vertical padding around knob
            auto levelKnobBounds = levelKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, levelKnobArea.getWidth()),
                juce::jlimit(minKnobSize, levelKnobArea.getHeight(), levelKnobArea.getHeight())
            );
            section.levelSlider.setBounds(levelKnobBounds);
            section.levelValue.setBounds(levelValueArea);

            // Detune knob
            auto detuneArea = bottomRow.removeFromLeft(knobWidth).reduced(3);
            section.detuneLabel.setBounds(detuneArea.removeFromTop(minLabelHeight));
            auto detuneValueArea = detuneArea.removeFromBottom(minValueHeight);
            auto detuneKnobArea = detuneArea.reduced(0, 2); // Reduce vertical padding around knob
            auto detuneKnobBounds = detuneKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, detuneKnobArea.getWidth()),
                juce::jlimit(minKnobSize, detuneKnobArea.getHeight(), detuneKnobArea.getHeight())
            );
            section.detuneSlider.setBounds(detuneKnobBounds);
            section.detuneValue.setBounds(detuneValueArea);

            // Pan knob
            auto panArea = bottomRow.reduced(3);
            section.panLabel.setBounds(panArea.removeFromTop(minLabelHeight));
            auto panValueArea = panArea.removeFromBottom(minValueHeight);
            auto panKnobArea = panArea.reduced(0, 2); // Reduce vertical padding around knob
            auto panKnobBounds = panKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, panKnobArea.getWidth()),
                juce::jlimit(minKnobSize, panKnobArea.getHeight(), panKnobArea.getHeight())
            );
            section.panSlider.setBounds(panKnobBounds);
            section.panValue.setBounds(panValueArea);
        }

        void layoutGroupNoise(juce::Rectangle<int> area)
        {
            // Layout noise generator using GroupComponent
            owner.noiseGroup.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = owner.noiseGroup.getLocalBounds().reduced(8);
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight));
            
            // Two-row layout: Dropdown on top, knobs underneath (same as oscillators)
            auto topRowHeight = 40; // Reduced height for dropdown section
            auto topRow = bounds.removeFromTop(topRowHeight);
            auto bottomRow = bounds; // Remaining space for knobs
            
            // Top row: Type dropdown (full width) - no label
            auto typeArea = topRow.reduced(2);
            owner.noiseTypeCombo.setBounds(typeArea);

            // Bottom row: Level and Pan knobs horizontally
            auto knobWidth = bottomRow.getWidth() / 2; // Only 2 knobs for noise
            
            const int minKnobSize = 45; // Smaller knobs for noise generator (reduced from 55)
            const int minLabelHeight = 12; // Reduced label height
            const int minValueHeight = 12; // Reduced value height

            // Level knob (left half)
            auto levelArea = bottomRow.removeFromLeft(knobWidth).reduced(3);
            owner.noiseLevelLabel.setBounds(levelArea.removeFromTop(minLabelHeight));
            auto levelValueArea = levelArea.removeFromBottom(minValueHeight);
            auto levelKnobArea = levelArea.reduced(0, 2); // Reduce vertical padding around knob
            auto levelKnobBounds = levelKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, levelKnobArea.getWidth()),
                juce::jlimit(minKnobSize, levelKnobArea.getHeight(), levelKnobArea.getHeight())
            );
            owner.noiseLevelSlider.setBounds(levelKnobBounds);
            owner.noiseLevelValue.setBounds(levelValueArea);

            // Pan knob (right half)
            auto panArea = bottomRow.reduced(3);
            owner.noisePanLabel.setBounds(panArea.removeFromTop(minLabelHeight));
            auto panValueArea = panArea.removeFromBottom(minValueHeight);
            auto panKnobArea = panArea.reduced(0, 2); // Reduce vertical padding around knob
            auto panKnobBounds = panKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, panKnobArea.getWidth()),
                juce::jlimit(minKnobSize, panKnobArea.getHeight(), panKnobArea.getHeight())
            );
            owner.noisePanSlider.setBounds(panKnobBounds);
            owner.noisePanValue.setBounds(panValueArea);
        }
        
        void layoutGroupEnvelope(juce::Rectangle<int> area)
        {
            // Layout envelope generator using GroupComponent (matching noise generator)
            owner.envelopeGroup.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = owner.envelopeGroup.getLocalBounds().reduced(8);
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight));
            
            // Layout ADSR knobs in horizontal row: A D S R
            auto knobWidth = bounds.getWidth() / 4; // 4 knobs across
            
            const int minKnobSize = 55; // Same size as oscillator knobs
            const int minLabelHeight = 12;
            const int minValueHeight = 12;

            // Attack (leftmost)
            auto attackArea = bounds.removeFromLeft(knobWidth).reduced(3);
            owner.attackLabel.setBounds(attackArea.removeFromTop(minLabelHeight));
            auto attackValueArea = attackArea.removeFromBottom(minValueHeight);
            auto attackKnobArea = attackArea.reduced(0, 2);
            auto attackKnobBounds = attackKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, attackKnobArea.getWidth()),
                juce::jlimit(minKnobSize, attackKnobArea.getHeight(), attackKnobArea.getHeight())
            );
            owner.attackSlider.setBounds(attackKnobBounds);
            owner.attackValue.setBounds(attackValueArea);

            // Decay (second from left)
            auto decayArea = bounds.removeFromLeft(knobWidth).reduced(3);
            owner.decayLabel.setBounds(decayArea.removeFromTop(minLabelHeight));
            auto decayValueArea = decayArea.removeFromBottom(minValueHeight);
            auto decayKnobArea = decayArea.reduced(0, 2);
            auto decayKnobBounds = decayKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, decayKnobArea.getWidth()),
                juce::jlimit(minKnobSize, decayKnobArea.getHeight(), decayKnobArea.getHeight())
            );
            owner.decaySlider.setBounds(decayKnobBounds);
            owner.decayValue.setBounds(decayValueArea);

            // Sustain (third from left)
            auto sustainArea = bounds.removeFromLeft(knobWidth).reduced(3);
            owner.sustainLabel.setBounds(sustainArea.removeFromTop(minLabelHeight));
            auto sustainValueArea = sustainArea.removeFromBottom(minValueHeight);
            auto sustainKnobArea = sustainArea.reduced(0, 2);
            auto sustainKnobBounds = sustainKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, sustainKnobArea.getWidth()),
                juce::jlimit(minKnobSize, sustainKnobArea.getHeight(), sustainKnobArea.getHeight())
            );
            owner.sustainSlider.setBounds(sustainKnobBounds);
            owner.sustainValue.setBounds(sustainValueArea);

            // Release (rightmost)
            auto releaseArea = bounds.reduced(3); // Remaining space
            owner.releaseLabel.setBounds(releaseArea.removeFromTop(minLabelHeight));
            auto releaseValueArea = releaseArea.removeFromBottom(minValueHeight);
            auto releaseKnobArea = releaseArea.reduced(0, 2);
            auto releaseKnobBounds = releaseKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, releaseKnobArea.getWidth()),
                juce::jlimit(minKnobSize, releaseKnobArea.getHeight(), releaseKnobArea.getHeight())
            );
            owner.releaseSlider.setBounds(releaseKnobBounds);
            owner.releaseValue.setBounds(releaseValueArea);
        }

        void layoutDirectOscillator(FreOscEditor::OscillatorSection& section, juce::Rectangle<int> area, const juce::String& title)
        {
            juce::ignoreUnused(title); // Parameter no longer used after removing custom drawing
            
            // DIRECT LAYOUT: Position components directly in the given area
            // This is what we proved works with our test components

            // Store the area for border drawing (add some padding for the border)
            // BorderArea assignments removed - no longer needed

            // Create a title area (like GroupComponent border) - leave space at top for title
            auto titleArea = area.removeFromTop(20);

            // Top row: Waveform and Octave dropdowns side by side
            auto topRow = area.removeFromTop(50).reduced(5);
            auto waveformArea = topRow.removeFromLeft(topRow.getWidth() / 2).reduced(2);
            auto octaveArea = topRow.reduced(2);

            // Waveform
            section.waveformLabel.setBounds(waveformArea.removeFromTop(15));
            section.waveformSelector.setBounds(waveformArea);

            // Octave
            section.octaveLabel.setBounds(octaveArea.removeFromTop(15));
            section.octaveSelector.setBounds(octaveArea);

            // Bottom row: Level, Detune, Pan knobs side by side with minimum size constraints
            auto bottomRow = area.reduced(3); // Less padding

            // Calculate minimum space needed for each control - more compact
            const int minKnobSize = 40;  // Smaller knobs
            const int minLabelHeight = 12; // Smaller labels
            const int minValueHeight = 12; // Smaller value displays
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
            // noiseBorderArea assignment removed - no longer needed

            // Leave space at top for title
            area.removeFromTop(20);

            // Layout: Type dropdown on top, then Level and Pan knobs side by side
            auto topRow = area.removeFromTop(40).reduced(3); // More compact

            // Type dropdown (full width)
            owner.noiseTypeLabel.setBounds(topRow.removeFromTop(15));
            owner.noiseTypeCombo.setBounds(topRow);

            // Bottom row: Level and Pan knobs side by side
            auto bottomRow = area.reduced(3); // Less padding

            // Calculate minimum space needed for each control - compact
            const int minKnobSize = 40; // Smaller knobs
            const int minLabelHeight = 12;
            const int minValueHeight = 12;
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

        void paint(juce::Graphics& g) override
        {
            // Custom drawing removed - now using GroupComponent styling
            juce::ignoreUnused(g);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(5);

            // Layout oscillators horizontally in top section
            auto oscillatorsHeight = bounds.getHeight() * 0.6f; // Reduced to 60% for oscillators
            auto oscillatorsArea = bounds.removeFromTop(static_cast<int>(oscillatorsHeight));
            auto oscWidth = oscillatorsArea.getWidth() / 3;

            // Layout all 3 oscillators horizontally using GroupComponents
            auto osc1Area = oscillatorsArea.removeFromLeft(oscWidth).reduced(3);
            layoutGroupOscillator(owner.osc1Section, osc1Area);

            auto osc2Area = oscillatorsArea.removeFromLeft(oscWidth).reduced(3);
            layoutGroupOscillator(owner.osc2Section, osc2Area);

            auto osc3Area = oscillatorsArea.reduced(3);
            layoutGroupOscillator(owner.osc3Section, osc3Area);

            // Layout noise generator and envelope side by side in bottom area
            auto bottomArea = bounds.reduced(3);
            auto halfWidth = bottomArea.getWidth() / 2;
            
            auto noiseArea = bottomArea.removeFromLeft(halfWidth).reduced(3);
            layoutGroupNoise(noiseArea);
            
            auto envelopeArea = bottomArea.reduced(3);
            layoutGroupEnvelope(envelopeArea);
        }

    private:
        FreOscEditor& owner;

        // Border areas for drawing oscillator borders
        // BorderArea variables removed - no longer needed for custom drawing
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
            // GROUP COMPONENTS: Setup three separate filter groups
            setupGroupFilterRouting();
            setupGroupFilter1();
            setupGroupFilter2();
        }

        void setupGroupFilterRouting()
        {
            // Setup filter routing using GroupComponent
            owner.applyComponentStyling(owner.filterRoutingGroup);
            owner.filterRoutingGroup.setText("Routing");
            
            // Setup routing components
            owner.applyComponentStyling(owner.filterRoutingCombo);
            
            // Add components to the routing group (no label)
            owner.filterRoutingGroup.addAndMakeVisible(owner.filterRoutingCombo);
            
            // Add the group to the tab
            addAndMakeVisible(owner.filterRoutingGroup);
        }
        
        void setupGroupFilter1()
        {
            // Setup Filter 1 using GroupComponent
            owner.applyComponentStyling(owner.filter1Group);
            owner.filter1Group.setText("Filter 1");
            
            // Setup Filter 1 components
            owner.applyComponentStyling(owner.filterTypeCombo);
            
            // Set slider names for vertical styling
            owner.cutoffSlider.setName("filter_cutoff");
            owner.resonanceSlider.setName("filter_resonance");
            owner.filterGainSlider.setName("filter_gain");
            
            owner.applyComponentStyling(owner.cutoffSlider);
            owner.applyComponentStyling(owner.resonanceSlider);
            owner.applyComponentStyling(owner.filterGainSlider);
            
            // Skip filterTypeLabel - no label for dropdown
            owner.applyComponentStyling(owner.cutoffLabel);
            owner.applyComponentStyling(owner.resonanceLabel);
            owner.applyComponentStyling(owner.filterGainLabel);
            owner.applyComponentStyling(owner.cutoffValue);
            owner.applyComponentStyling(owner.resonanceValue);
            owner.applyComponentStyling(owner.filterGainValue);
            
            // Set value label styling with white text
            owner.cutoffValue.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.cutoffValue.setJustificationType(juce::Justification::centred);
            owner.cutoffValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.cutoffValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.resonanceValue.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.resonanceValue.setJustificationType(juce::Justification::centred);
            owner.resonanceValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.resonanceValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.filterGainValue.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.filterGainValue.setJustificationType(juce::Justification::centred);
            owner.filterGainValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.filterGainValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            
            // Set labels
            owner.cutoffLabel.setText("Cutoff", juce::dontSendNotification);
            owner.resonanceLabel.setText("Resonance", juce::dontSendNotification);
            owner.filterGainLabel.setText("Gain", juce::dontSendNotification);
            
            // Add Filter 1 components to the group (no type label)
            owner.filter1Group.addAndMakeVisible(owner.filterTypeCombo);
            owner.filter1Group.addAndMakeVisible(owner.cutoffLabel);
            owner.filter1Group.addAndMakeVisible(owner.cutoffSlider);
            owner.filter1Group.addAndMakeVisible(owner.cutoffValue);
            owner.filter1Group.addAndMakeVisible(owner.resonanceLabel);
            owner.filter1Group.addAndMakeVisible(owner.resonanceSlider);
            owner.filter1Group.addAndMakeVisible(owner.resonanceValue);
            owner.filter1Group.addAndMakeVisible(owner.filterGainLabel);
            owner.filter1Group.addAndMakeVisible(owner.filterGainSlider);
            owner.filter1Group.addAndMakeVisible(owner.filterGainValue);
            
            // Add the group to the tab
            addAndMakeVisible(owner.filter1Group);
        }
        
        void setupGroupFilter2()
        {
            // Setup Filter 2 using GroupComponent
            owner.applyComponentStyling(owner.filter2Group);
            owner.filter2Group.setText("Filter 2");
            
            // Setup Filter 2 components
            owner.applyComponentStyling(owner.filter2TypeCombo);
            
            // Set slider names for vertical styling
            owner.cutoff2Slider.setName("filter2_cutoff");
            owner.resonance2Slider.setName("filter2_resonance");
            owner.filterGain2Slider.setName("filter2_gain");
            
            owner.applyComponentStyling(owner.cutoff2Slider);
            owner.applyComponentStyling(owner.resonance2Slider);
            owner.applyComponentStyling(owner.filterGain2Slider);
            
            // Skip filter2TypeLabel - no label for dropdown
            owner.applyComponentStyling(owner.cutoff2Label);
            owner.applyComponentStyling(owner.resonance2Label);
            owner.applyComponentStyling(owner.filterGain2Label);
            owner.applyComponentStyling(owner.cutoff2Value);
            owner.applyComponentStyling(owner.resonance2Value);
            owner.applyComponentStyling(owner.filterGain2Value);
            
            // Set value label styling with white text
            owner.cutoff2Value.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.cutoff2Value.setJustificationType(juce::Justification::centred);
            owner.cutoff2Value.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.cutoff2Value.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.resonance2Value.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.resonance2Value.setJustificationType(juce::Justification::centred);
            owner.resonance2Value.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.resonance2Value.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.filterGain2Value.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.filterGain2Value.setJustificationType(juce::Justification::centred);
            owner.filterGain2Value.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.filterGain2Value.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            
            // Set labels
            owner.cutoff2Label.setText("Cutoff", juce::dontSendNotification);
            owner.resonance2Label.setText("Resonance", juce::dontSendNotification);
            owner.filterGain2Label.setText("Gain", juce::dontSendNotification);
            
            // Add Filter 2 components to the group (no type label)
            owner.filter2Group.addAndMakeVisible(owner.filter2TypeCombo);
            owner.filter2Group.addAndMakeVisible(owner.cutoff2Label);
            owner.filter2Group.addAndMakeVisible(owner.cutoff2Slider);
            owner.filter2Group.addAndMakeVisible(owner.cutoff2Value);
            owner.filter2Group.addAndMakeVisible(owner.resonance2Label);
            owner.filter2Group.addAndMakeVisible(owner.resonance2Slider);
            owner.filter2Group.addAndMakeVisible(owner.resonance2Value);
            owner.filter2Group.addAndMakeVisible(owner.filterGain2Label);
            owner.filter2Group.addAndMakeVisible(owner.filterGain2Slider);
            owner.filter2Group.addAndMakeVisible(owner.filterGain2Value);
            
            // Add the group to the tab
            addAndMakeVisible(owner.filter2Group);
        }

        void setupDirectFilter()
        {
            // DIRECT SETUP: Add filter components directly to tab (no GroupComponent)

            // Setup components with proper styling
            owner.applyComponentStyling(owner.filterTypeCombo);
            owner.applyComponentStyling(owner.cutoffSlider);
            owner.applyComponentStyling(owner.resonanceSlider);
            owner.applyComponentStyling(owner.filterGainSlider);
            owner.applyComponentStyling(owner.filterTypeLabel);
            owner.applyComponentStyling(owner.cutoffLabel);
            owner.applyComponentStyling(owner.resonanceLabel);
            owner.applyComponentStyling(owner.filterGainLabel);
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

            // Filter combo box options are set up in setupComboBoxOptions() - no need to duplicate here

            // Set labels
            owner.cutoffLabel.setText("Cutoff", juce::dontSendNotification);
            owner.resonanceLabel.setText("Resonance", juce::dontSendNotification);
            owner.filterGainLabel.setText("Gain", juce::dontSendNotification);

            // Add ALL components directly to the tab (not to a group) - INCLUDING VALUE LABELS
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
        }

        void setupGroupEnvelope()
        {
            // Setup envelope generator using GroupComponent (to match noise generator styling)
            owner.applyComponentStyling(owner.envelopeGroup);
            owner.envelopeGroup.setText("Envelope");
            
            // Setup and add all components to the group
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

            // Set value label styling for envelope generator with white text
            owner.attackValue.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.attackValue.setJustificationType(juce::Justification::centred);
            owner.attackValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.attackValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.decayValue.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.decayValue.setJustificationType(juce::Justification::centred);
            owner.decayValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.decayValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.sustainValue.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.sustainValue.setJustificationType(juce::Justification::centred);
            owner.sustainValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.sustainValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.releaseValue.setFont(juce::Font(juce::FontOptions(11.0f)));
            owner.releaseValue.setJustificationType(juce::Justification::centred);
            owner.releaseValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.releaseValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // Set labels
            owner.attackLabel.setText("Attack", juce::dontSendNotification);
            owner.decayLabel.setText("Decay", juce::dontSendNotification);
            owner.sustainLabel.setText("Sustain", juce::dontSendNotification);
            owner.releaseLabel.setText("Release", juce::dontSendNotification);

            // Add ALL components to the group
            owner.envelopeGroup.addAndMakeVisible(owner.attackLabel);
            owner.envelopeGroup.addAndMakeVisible(owner.attackSlider);
            owner.envelopeGroup.addAndMakeVisible(owner.attackValue);
            owner.envelopeGroup.addAndMakeVisible(owner.decayLabel);
            owner.envelopeGroup.addAndMakeVisible(owner.decaySlider);
            owner.envelopeGroup.addAndMakeVisible(owner.decayValue);
            owner.envelopeGroup.addAndMakeVisible(owner.sustainLabel);
            owner.envelopeGroup.addAndMakeVisible(owner.sustainSlider);
            owner.envelopeGroup.addAndMakeVisible(owner.sustainValue);
            owner.envelopeGroup.addAndMakeVisible(owner.releaseLabel);
            owner.envelopeGroup.addAndMakeVisible(owner.releaseSlider);
            owner.envelopeGroup.addAndMakeVisible(owner.releaseValue);

            // Add the group to the tab
            addAndMakeVisible(owner.envelopeGroup);
        }

        void layoutDirectFilter(juce::Rectangle<int> area)
        {
            // Store area for border drawing
            // filterBorderArea assignment removed - no longer needed

            // Layout: Type dropdown on top, then sliders
            auto topRow = area.removeFromTop(35).reduced(3); // Larger dropdown area

            // Type dropdown (full width, no label)
            owner.filterTypeCombo.setBounds(topRow);

            // Bottom area: Cutoff, Resonance, Gain knobs
            auto bottomRow = area.reduced(3); // Less padding
            auto knobWidth = bottomRow.getWidth() / 3;

            // Minimum knob size - compact
            const int minKnobSize = 45; // Smaller knobs
            const int minLabelHeight = 12;

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

        void paint(juce::Graphics& g) override
        {
            // Custom drawing removed - now using GroupComponent styling
            juce::ignoreUnused(g);
        }

        void layoutGroupFilterRouting(juce::Rectangle<int> area)
        {
            // Layout filter routing using GroupComponent
            owner.filterRoutingGroup.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = owner.filterRoutingGroup.getLocalBounds().reduced(8);
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight));
            
            // Simple layout: combo only (no label)
            owner.filterRoutingCombo.setBounds(bounds.reduced(2));
        }
        
        void layoutGroupFilter1(juce::Rectangle<int> area)
        {
            // Layout Filter 1 using GroupComponent
            owner.filter1Group.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = owner.filter1Group.getLocalBounds().reduced(8);
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight));
            
            // Layout single filter
            layoutSingleFilter(bounds, 
                              owner.filterTypeCombo, 
                              owner.cutoffSlider, owner.resonanceSlider, owner.filterGainSlider,
                              owner.cutoffLabel, owner.resonanceLabel, owner.filterGainLabel,
                              owner.cutoffValue, owner.resonanceValue, owner.filterGainValue);
        }
        
        void layoutGroupFilter2(juce::Rectangle<int> area)
        {
            // Layout Filter 2 using GroupComponent
            owner.filter2Group.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = owner.filter2Group.getLocalBounds().reduced(8);
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight));
            
            // Layout single filter
            layoutSingleFilter(bounds,
                              owner.filter2TypeCombo,
                              owner.cutoff2Slider, owner.resonance2Slider, owner.filterGain2Slider,
                              owner.cutoff2Label, owner.resonance2Label, owner.filterGain2Label,
                              owner.cutoff2Value, owner.resonance2Value, owner.filterGain2Value);
        }
        
        void layoutSingleFilter(juce::Rectangle<int> area,
                               juce::ComboBox& typeCombo,
                               juce::Slider& cutoffSlider, juce::Slider& resonanceSlider, juce::Slider& gainSlider,
                               juce::Label& cutoffLabel, juce::Label& resonanceLabel, juce::Label& gainLabel,
                               juce::Label& cutoffValue, juce::Label& resonanceValue, juce::Label& gainValue)
        {
            // Type dropdown on top (no label)
            auto topRowHeight = 25;  // Reduced since no label
            auto topRow = area.removeFromTop(topRowHeight);
            typeCombo.setBounds(topRow.reduced(2));
            
            // Three vertical sliders underneath
            auto sliderRow = area;
            auto sliderWidth = sliderRow.getWidth() / 3;
            
            const int minLabelHeight = 12;
            const int minValueHeight = 12;
            
            // Cutoff slider (left)
            auto cutoffArea = sliderRow.removeFromLeft(sliderWidth).reduced(2);
            cutoffLabel.setBounds(cutoffArea.removeFromTop(minLabelHeight));
            auto cutoffValueArea = cutoffArea.removeFromBottom(minValueHeight);
            cutoffSlider.setBounds(cutoffArea);
            cutoffValue.setBounds(cutoffValueArea);
            
            // Resonance slider (center)
            auto resonanceArea = sliderRow.removeFromLeft(sliderWidth).reduced(2);
            resonanceLabel.setBounds(resonanceArea.removeFromTop(minLabelHeight));
            auto resonanceValueArea = resonanceArea.removeFromBottom(minValueHeight);
            resonanceSlider.setBounds(resonanceArea);
            resonanceValue.setBounds(resonanceValueArea);
            
            // Gain slider (right)
            auto gainArea = sliderRow.reduced(2);
            gainLabel.setBounds(gainArea.removeFromTop(minLabelHeight));
            auto gainValueArea = gainArea.removeFromBottom(minValueHeight);
            gainSlider.setBounds(gainArea);
            gainValue.setBounds(gainValueArea);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(5);

            // Three-section layout: Routing on top, Filter 1 and Filter 2 side by side below
            auto routingHeight = 80;  // Small area for routing
            auto routingArea = bounds.removeFromTop(routingHeight).reduced(3);
            
            // Remaining area for filters side by side
            auto filtersArea = bounds.reduced(3);
            auto halfWidth = filtersArea.getWidth() / 2;
            auto filter1Area = filtersArea.removeFromLeft(halfWidth).reduced(3);
            auto filter2Area = filtersArea.reduced(3);

            // Layout the three groups
            layoutGroupFilterRouting(routingArea);
            layoutGroupFilter1(filter1Area);
            layoutGroupFilter2(filter2Area);
        }

    private:
        FreOscEditor& owner;

        // Border areas for drawing section borders
        // filterBorderArea variable removed - no longer needed for custom drawing
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
            // GROUP LAYOUT: Setup LFO, FM, and Modulation Envelope components with GroupComponent
            setupGroupLFO();
            setupGroupPM();
            setupGroupModEnv1();
            setupGroupModEnv2();
        }

        void setupGroupLFO()
        {
            // GROUP SETUP: Add LFO components to GroupComponent
            // Set text FIRST so applyComponentStyling can detect it's the LFO group
            owner.lfoGroup.setText("LFO");
            owner.applyComponentStyling(owner.lfoGroup);
            
            // LFO color now handled by LookAndFeel - no manual color setting needed

            // Setup components with proper styling
            // Note: lfoWaveformSelector doesn't need applyComponentStyling as it's custom
            owner.applyComponentStyling(owner.lfoTargetCombo);
            
            // Setup LFO sliders as vertical sliders with LEDs (like oscillator sliders)
            owner.lfoRateSlider.setName("LFO_rate");
            owner.lfoAmountSlider.setName("LFO_amount");
            owner.applyComponentStyling(owner.lfoRateSlider);
            owner.applyComponentStyling(owner.lfoAmountSlider);

            owner.applyComponentStyling(owner.lfoWaveformLabel);
            owner.applyComponentStyling(owner.lfoTargetLabel);
            owner.applyComponentStyling(owner.lfoRateLabel);
            owner.applyComponentStyling(owner.lfoAmountLabel);
            owner.applyComponentStyling(owner.lfoRateValue);
            owner.applyComponentStyling(owner.lfoAmountValue);

            // Set value label styling for LFO controls (white like other value labels)
            owner.lfoRateValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.lfoRateValue.setJustificationType(juce::Justification::centred);
            owner.lfoRateValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.lfoRateValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.lfoAmountValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.lfoAmountValue.setJustificationType(juce::Justification::centred);
            owner.lfoAmountValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.lfoAmountValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // Set labels
            owner.lfoWaveformLabel.setText("Waveform", juce::dontSendNotification);
            owner.lfoTargetLabel.setText("Target", juce::dontSendNotification);
            owner.lfoRateLabel.setText("Rate", juce::dontSendNotification);
            owner.lfoAmountLabel.setText("Amount", juce::dontSendNotification);

            // Add components to the group
            owner.lfoGroup.addAndMakeVisible(owner.lfoWaveformLabel);
            owner.lfoGroup.addAndMakeVisible(owner.lfoWaveformSelector);
            owner.lfoGroup.addAndMakeVisible(owner.lfoTargetLabel);
            owner.lfoGroup.addAndMakeVisible(owner.lfoTargetCombo);
            owner.lfoGroup.addAndMakeVisible(owner.lfoRateLabel);
            owner.lfoGroup.addAndMakeVisible(owner.lfoRateSlider);
            owner.lfoGroup.addAndMakeVisible(owner.lfoRateValue);
            owner.lfoGroup.addAndMakeVisible(owner.lfoAmountLabel);
            owner.lfoGroup.addAndMakeVisible(owner.lfoAmountSlider);
            owner.lfoGroup.addAndMakeVisible(owner.lfoAmountValue);

            // Add the group to the tab
            addAndMakeVisible(owner.lfoGroup);
            
            // Ensure custom LookAndFeel is applied to this tab component
            setLookAndFeel(owner.customLookAndFeel.get());
        }

        void setupGroupPM()
        {
            // GROUP SETUP: Add FM components to GroupComponent
            owner.applyComponentStyling(owner.pmGroup);
            owner.pmGroup.setText("PM");

            // Setup components with proper styling
            owner.applyComponentStyling(owner.pmCarrierCombo);
            owner.applyComponentStyling(owner.pmIndexSlider);
            owner.applyComponentStyling(owner.pmRatioSlider);

            owner.applyComponentStyling(owner.pmCarrierLabel);
            owner.applyComponentStyling(owner.pmIndexLabel);
            owner.applyComponentStyling(owner.pmRatioLabel);
            owner.applyComponentStyling(owner.pmIndexValue);
            owner.applyComponentStyling(owner.pmRatioValue);

            // Set value label styling for PM controls (white text)
            owner.pmIndexValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.pmIndexValue.setJustificationType(juce::Justification::centred);
            owner.pmIndexValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.pmIndexValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            owner.pmRatioValue.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
            owner.pmRatioValue.setJustificationType(juce::Justification::centred);
            owner.pmRatioValue.setColour(juce::Label::textColourId, juce::Colours::white);
            owner.pmRatioValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            // Set labels
            owner.pmCarrierLabel.setText("Carrier", juce::dontSendNotification);
            owner.pmIndexLabel.setText("Index", juce::dontSendNotification);
            owner.pmRatioLabel.setText("Ratio", juce::dontSendNotification);

            // Add components to the group
            owner.pmGroup.addAndMakeVisible(owner.pmCarrierLabel);
            owner.pmGroup.addAndMakeVisible(owner.pmCarrierCombo);
            owner.pmGroup.addAndMakeVisible(owner.pmIndexLabel);
            owner.pmGroup.addAndMakeVisible(owner.pmIndexSlider);
            owner.pmGroup.addAndMakeVisible(owner.pmIndexValue);
            owner.pmGroup.addAndMakeVisible(owner.pmRatioLabel);
            owner.pmGroup.addAndMakeVisible(owner.pmRatioSlider);
            owner.pmGroup.addAndMakeVisible(owner.pmRatioValue);

            // Add the group to the tab
            addAndMakeVisible(owner.pmGroup);
        }

        void setupGroupModEnv1()
        {
            // GROUP SETUP: Add ModEnv1 components to GroupComponent
            owner.applyComponentStyling(owner.modEnv1Group);
            owner.modEnv1Group.setText("Mod Env 1");

            // Setup components with proper styling
            owner.applyComponentStyling(owner.modEnv1TargetCombo);
            owner.applyComponentStyling(owner.modEnv1ModeCombo);
            owner.applyComponentStyling(owner.modEnv1AttackSlider);
            owner.applyComponentStyling(owner.modEnv1DecaySlider);
            owner.applyComponentStyling(owner.modEnv1SustainSlider);
            owner.applyComponentStyling(owner.modEnv1ReleaseSlider);
            owner.applyComponentStyling(owner.modEnv1AmountSlider);
            owner.applyComponentStyling(owner.modEnv1RateSlider);

            owner.applyComponentStyling(owner.modEnv1TargetLabel);
            owner.applyComponentStyling(owner.modEnv1ModeLabel);
            owner.applyComponentStyling(owner.modEnv1AttackLabel);
            owner.applyComponentStyling(owner.modEnv1DecayLabel);
            owner.applyComponentStyling(owner.modEnv1SustainLabel);
            owner.applyComponentStyling(owner.modEnv1ReleaseLabel);
            owner.applyComponentStyling(owner.modEnv1AmountLabel);
            owner.applyComponentStyling(owner.modEnv1RateLabel);
            owner.applyComponentStyling(owner.modEnv1AttackValue);
            owner.applyComponentStyling(owner.modEnv1DecayValue);
            owner.applyComponentStyling(owner.modEnv1SustainValue);
            owner.applyComponentStyling(owner.modEnv1ReleaseValue);
            owner.applyComponentStyling(owner.modEnv1AmountValue);
            owner.applyComponentStyling(owner.modEnv1RateValue);

            // Set value label styling (white like other value labels)
            auto setValueLabelStyle = [](juce::Label& label) {
                label.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
                label.setJustificationType(juce::Justification::centred);
                label.setColour(juce::Label::textColourId, juce::Colours::white);
                label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            };

            setValueLabelStyle(owner.modEnv1AttackValue);
            setValueLabelStyle(owner.modEnv1DecayValue);
            setValueLabelStyle(owner.modEnv1SustainValue);
            setValueLabelStyle(owner.modEnv1ReleaseValue);
            setValueLabelStyle(owner.modEnv1AmountValue);
            setValueLabelStyle(owner.modEnv1RateValue);

            // Set labels
            owner.modEnv1TargetLabel.setText("Target", juce::dontSendNotification);
            owner.modEnv1ModeLabel.setText("Mode", juce::dontSendNotification);
            owner.modEnv1AttackLabel.setText("Attack", juce::dontSendNotification);
            owner.modEnv1DecayLabel.setText("Decay", juce::dontSendNotification);
            owner.modEnv1SustainLabel.setText("Sustain", juce::dontSendNotification);
            owner.modEnv1ReleaseLabel.setText("Release", juce::dontSendNotification);
            owner.modEnv1AmountLabel.setText("Amount", juce::dontSendNotification);
            owner.modEnv1RateLabel.setText("Rate", juce::dontSendNotification);

            // Add components to the group
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1TargetLabel);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1TargetCombo);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1ModeLabel);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1ModeCombo);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1AttackLabel);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1AttackSlider);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1AttackValue);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1DecayLabel);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1DecaySlider);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1DecayValue);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1SustainLabel);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1SustainSlider);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1SustainValue);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1ReleaseLabel);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1ReleaseSlider);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1ReleaseValue);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1AmountLabel);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1AmountSlider);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1AmountValue);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1RateLabel);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1RateSlider);
            owner.modEnv1Group.addAndMakeVisible(owner.modEnv1RateValue);

            // Add the group to the tab
            addAndMakeVisible(owner.modEnv1Group);
        }

        void setupGroupModEnv2()
        {
            // GROUP SETUP: Add ModEnv2 components to GroupComponent
            owner.applyComponentStyling(owner.modEnv2Group);
            owner.modEnv2Group.setText("Mod Env 2");

            // Setup components with proper styling
            owner.applyComponentStyling(owner.modEnv2TargetCombo);
            owner.applyComponentStyling(owner.modEnv2ModeCombo);
            owner.applyComponentStyling(owner.modEnv2AttackSlider);
            owner.applyComponentStyling(owner.modEnv2DecaySlider);
            owner.applyComponentStyling(owner.modEnv2SustainSlider);
            owner.applyComponentStyling(owner.modEnv2ReleaseSlider);
            owner.applyComponentStyling(owner.modEnv2AmountSlider);
            owner.applyComponentStyling(owner.modEnv2RateSlider);

            owner.applyComponentStyling(owner.modEnv2TargetLabel);
            owner.applyComponentStyling(owner.modEnv2ModeLabel);
            owner.applyComponentStyling(owner.modEnv2AttackLabel);
            owner.applyComponentStyling(owner.modEnv2DecayLabel);
            owner.applyComponentStyling(owner.modEnv2SustainLabel);
            owner.applyComponentStyling(owner.modEnv2ReleaseLabel);
            owner.applyComponentStyling(owner.modEnv2AmountLabel);
            owner.applyComponentStyling(owner.modEnv2RateLabel);
            owner.applyComponentStyling(owner.modEnv2AttackValue);
            owner.applyComponentStyling(owner.modEnv2DecayValue);
            owner.applyComponentStyling(owner.modEnv2SustainValue);
            owner.applyComponentStyling(owner.modEnv2ReleaseValue);
            owner.applyComponentStyling(owner.modEnv2AmountValue);
            owner.applyComponentStyling(owner.modEnv2RateValue);

            // Set value label styling (white like other value labels)
            auto setValueLabelStyle = [](juce::Label& label) {
                label.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
                label.setJustificationType(juce::Justification::centred);
                label.setColour(juce::Label::textColourId, juce::Colours::white);
                label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            };

            setValueLabelStyle(owner.modEnv2AttackValue);
            setValueLabelStyle(owner.modEnv2DecayValue);
            setValueLabelStyle(owner.modEnv2SustainValue);
            setValueLabelStyle(owner.modEnv2ReleaseValue);
            setValueLabelStyle(owner.modEnv2AmountValue);
            setValueLabelStyle(owner.modEnv2RateValue);

            // Set labels
            owner.modEnv2TargetLabel.setText("Target", juce::dontSendNotification);
            owner.modEnv2ModeLabel.setText("Mode", juce::dontSendNotification);
            owner.modEnv2AttackLabel.setText("Attack", juce::dontSendNotification);
            owner.modEnv2DecayLabel.setText("Decay", juce::dontSendNotification);
            owner.modEnv2SustainLabel.setText("Sustain", juce::dontSendNotification);
            owner.modEnv2ReleaseLabel.setText("Release", juce::dontSendNotification);
            owner.modEnv2AmountLabel.setText("Amount", juce::dontSendNotification);
            owner.modEnv2RateLabel.setText("Rate", juce::dontSendNotification);

            // Add components to the group
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2TargetLabel);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2TargetCombo);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2ModeLabel);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2ModeCombo);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2AttackLabel);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2AttackSlider);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2AttackValue);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2DecayLabel);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2DecaySlider);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2DecayValue);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2SustainLabel);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2SustainSlider);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2SustainValue);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2ReleaseLabel);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2ReleaseSlider);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2ReleaseValue);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2AmountLabel);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2AmountSlider);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2AmountValue);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2RateLabel);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2RateSlider);
            owner.modEnv2Group.addAndMakeVisible(owner.modEnv2RateValue);

            // Add the group to the tab
            addAndMakeVisible(owner.modEnv2Group);
        }

        void layoutGroupLFO(juce::Rectangle<int> area)
        {
            // Layout LFO using GroupComponent
            owner.lfoGroup.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = owner.lfoGroup.getLocalBounds().reduced(8);
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight));
            
            // Two-row layout: Dropdowns on top, knobs underneath
            auto topRowHeight = 40;
            auto topRow = bounds.removeFromTop(topRowHeight);
            auto bottomRow = bounds;
            
            // Top row: Waveform and Target dropdowns side by side
            auto dropdownWidth = topRow.getWidth() / 2;
            
            // Waveform (left half)
            auto waveformArea = topRow.removeFromLeft(dropdownWidth).reduced(2);
            owner.lfoWaveformLabel.setBounds(waveformArea.removeFromTop(15));
            owner.lfoWaveformSelector.setBounds(waveformArea);

            // Target (right half)
            auto targetArea = topRow.reduced(2);
            owner.lfoTargetLabel.setBounds(targetArea.removeFromTop(15));
            owner.lfoTargetCombo.setBounds(targetArea);
            
            // Bottom row: Rate and Amount vertical sliders horizontally (like oscillator sliders)
            auto sliderWidth = bottomRow.getWidth() / 2;
            
            const int minLabelHeight = 12;
            const int minValueHeight = 12;
            
            // Rate slider (left)
            auto rateArea = bottomRow.removeFromLeft(sliderWidth).reduced(3);
            owner.lfoRateLabel.setBounds(rateArea.removeFromTop(minLabelHeight));
            auto rateValueArea = rateArea.removeFromBottom(minValueHeight);
            auto rateSliderArea = rateArea.reduced(2);
            
            // Give full area to slider (like oscillator sliders) so LEDs have space
            owner.lfoRateSlider.setBounds(rateSliderArea);
            owner.lfoRateValue.setBounds(rateValueArea);
            
            // Amount slider (right)
            auto amountArea = bottomRow.reduced(3);
            owner.lfoAmountLabel.setBounds(amountArea.removeFromTop(minLabelHeight));
            auto amountValueArea = amountArea.removeFromBottom(minValueHeight);
            auto amountSliderArea = amountArea.reduced(2);
            
            // Give full area to slider (like oscillator sliders) so LEDs have space
            owner.lfoAmountSlider.setBounds(amountSliderArea);
            owner.lfoAmountValue.setBounds(amountValueArea);
        }
        

        void layoutGroupPM(juce::Rectangle<int> area)
        {
            // Layout PM using GroupComponent
            owner.pmGroup.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = owner.pmGroup.getLocalBounds().reduced(8);
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight));
            
            // Two-row layout: Target dropdown on top, knobs underneath
            auto topRowHeight = 40;
            auto topRow = bounds.removeFromTop(topRowHeight);
            auto bottomRow = bounds;
            
            // Top row: Target dropdown (full width)
            auto targetArea = topRow.reduced(2);
            owner.pmCarrierLabel.setBounds(targetArea.removeFromTop(15));
            owner.pmCarrierCombo.setBounds(targetArea);
            
            // Bottom row: Amount and Ratio knobs horizontally
            auto knobWidth = bottomRow.getWidth() / 2;
            
            const int minKnobSize = 45;
            const int minLabelHeight = 12;
            const int minValueHeight = 12;
            
            // Amount knob (left)
            auto amountArea = bottomRow.removeFromLeft(knobWidth).reduced(3);
            owner.pmIndexLabel.setBounds(amountArea.removeFromTop(minLabelHeight));
            auto amountValueArea = amountArea.removeFromBottom(minValueHeight);
            auto amountKnobArea = amountArea.reduced(0, 2);
            auto amountKnobBounds = amountKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, amountKnobArea.getWidth()),
                juce::jlimit(minKnobSize, amountKnobArea.getHeight(), amountKnobArea.getHeight())
            );
            owner.pmIndexSlider.setBounds(amountKnobBounds);
            owner.pmIndexValue.setBounds(amountValueArea);
            
            // Ratio knob (right)
            auto ratioArea = bottomRow.reduced(3);
            owner.pmRatioLabel.setBounds(ratioArea.removeFromTop(minLabelHeight));
            auto ratioValueArea = ratioArea.removeFromBottom(minValueHeight);
            auto ratioKnobArea = ratioArea.reduced(0, 2);
            auto ratioKnobBounds = ratioKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, ratioKnobArea.getWidth()),
                juce::jlimit(minKnobSize, ratioKnobArea.getHeight(), ratioKnobArea.getHeight())
            );
            owner.pmRatioSlider.setBounds(ratioKnobBounds);
            owner.pmRatioValue.setBounds(ratioValueArea);
        }

        void layoutGroupModEnv1(juce::Rectangle<int> area)
        {
            // Layout ModEnv1 using GroupComponent similar to main envelope
            owner.modEnv1Group.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = owner.modEnv1Group.getLocalBounds().reduced(8);
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight));
            
            // Two-row layout: Target dropdown on top, ADSR+Amount+Rate knobs underneath
            auto topRowHeight = 40;
            auto topRow = bounds.removeFromTop(topRowHeight);
            auto bottomRow = bounds;
            
            // Top row: Target and Mode dropdowns side by side
            auto topRowArea = topRow.reduced(2);
            auto halfWidth = topRowArea.getWidth() / 2;
            
            // Target dropdown (left half)
            auto targetArea = topRowArea.removeFromLeft(halfWidth).reduced(2, 0);
            owner.modEnv1TargetLabel.setBounds(targetArea.removeFromTop(15));
            owner.modEnv1TargetCombo.setBounds(targetArea);
            
            // Mode dropdown (right half)
            auto modeArea = topRowArea.reduced(2, 0);
            owner.modEnv1ModeLabel.setBounds(modeArea.removeFromTop(15));
            owner.modEnv1ModeCombo.setBounds(modeArea);
            
            // Bottom row: 6 knobs horizontally (A, D, S, R, Amount, Rate)
            auto knobWidth = bottomRow.getWidth() / 6;
            
            const int minKnobSize = 30;  // Smaller knobs to fit 6 in row
            const int minLabelHeight = 12;
            const int minValueHeight = 12;
            
            // Attack knob
            auto attackArea = bottomRow.removeFromLeft(knobWidth).reduced(2);
            owner.modEnv1AttackLabel.setBounds(attackArea.removeFromTop(minLabelHeight));
            auto attackValueArea = attackArea.removeFromBottom(minValueHeight);
            auto attackKnobArea = attackArea.reduced(0, 1);
            auto attackKnobBounds = attackKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 4, attackKnobArea.getWidth()),
                juce::jlimit(minKnobSize, attackKnobArea.getHeight(), attackKnobArea.getHeight())
            );
            owner.modEnv1AttackSlider.setBounds(attackKnobBounds);
            owner.modEnv1AttackValue.setBounds(attackValueArea);
            
            // Decay knob
            auto decayArea = bottomRow.removeFromLeft(knobWidth).reduced(2);
            owner.modEnv1DecayLabel.setBounds(decayArea.removeFromTop(minLabelHeight));
            auto decayValueArea = decayArea.removeFromBottom(minValueHeight);
            auto decayKnobArea = decayArea.reduced(0, 1);
            auto decayKnobBounds = decayKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 4, decayKnobArea.getWidth()),
                juce::jlimit(minKnobSize, decayKnobArea.getHeight(), decayKnobArea.getHeight())
            );
            owner.modEnv1DecaySlider.setBounds(decayKnobBounds);
            owner.modEnv1DecayValue.setBounds(decayValueArea);
            
            // Sustain knob
            auto sustainArea = bottomRow.removeFromLeft(knobWidth).reduced(2);
            owner.modEnv1SustainLabel.setBounds(sustainArea.removeFromTop(minLabelHeight));
            auto sustainValueArea = sustainArea.removeFromBottom(minValueHeight);
            auto sustainKnobArea = sustainArea.reduced(0, 1);
            auto sustainKnobBounds = sustainKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 4, sustainKnobArea.getWidth()),
                juce::jlimit(minKnobSize, sustainKnobArea.getHeight(), sustainKnobArea.getHeight())
            );
            owner.modEnv1SustainSlider.setBounds(sustainKnobBounds);
            owner.modEnv1SustainValue.setBounds(sustainValueArea);
            
            // Release knob
            auto releaseArea = bottomRow.removeFromLeft(knobWidth).reduced(2);
            owner.modEnv1ReleaseLabel.setBounds(releaseArea.removeFromTop(minLabelHeight));
            auto releaseValueArea = releaseArea.removeFromBottom(minValueHeight);
            auto releaseKnobArea = releaseArea.reduced(0, 1);
            auto releaseKnobBounds = releaseKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 4, releaseKnobArea.getWidth()),
                juce::jlimit(minKnobSize, releaseKnobArea.getHeight(), releaseKnobArea.getHeight())
            );
            owner.modEnv1ReleaseSlider.setBounds(releaseKnobBounds);
            owner.modEnv1ReleaseValue.setBounds(releaseValueArea);
            
            // Amount knob
            auto amountArea = bottomRow.removeFromLeft(knobWidth).reduced(2);
            owner.modEnv1AmountLabel.setBounds(amountArea.removeFromTop(minLabelHeight));
            auto amountValueArea = amountArea.removeFromBottom(minValueHeight);
            auto amountKnobArea = amountArea.reduced(0, 1);
            auto amountKnobBounds = amountKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 4, amountKnobArea.getWidth()),
                juce::jlimit(minKnobSize, amountKnobArea.getHeight(), amountKnobArea.getHeight())
            );
            owner.modEnv1AmountSlider.setBounds(amountKnobBounds);
            owner.modEnv1AmountValue.setBounds(amountValueArea);
            
            // Rate knob
            auto rateArea = bottomRow.reduced(2);
            owner.modEnv1RateLabel.setBounds(rateArea.removeFromTop(minLabelHeight));
            auto rateValueArea = rateArea.removeFromBottom(minValueHeight);
            auto rateKnobArea = rateArea.reduced(0, 1);
            auto rateKnobBounds = rateKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 4, rateKnobArea.getWidth()),
                juce::jlimit(minKnobSize, rateKnobArea.getHeight(), rateKnobArea.getHeight())
            );
            owner.modEnv1RateSlider.setBounds(rateKnobBounds);
            owner.modEnv1RateValue.setBounds(rateValueArea);
        }

        void layoutGroupModEnv2(juce::Rectangle<int> area)
        {
            // Layout ModEnv2 using GroupComponent identical to ModEnv1
            owner.modEnv2Group.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = owner.modEnv2Group.getLocalBounds().reduced(8);
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight));
            
            // Two-row layout: Target dropdown on top, ADSR+Amount+Rate knobs underneath
            auto topRowHeight = 40;
            auto topRow = bounds.removeFromTop(topRowHeight);
            auto bottomRow = bounds;
            
            // Top row: Target and Mode dropdowns side by side
            auto topRowArea = topRow.reduced(2);
            auto halfWidth = topRowArea.getWidth() / 2;
            
            // Target dropdown (left half)
            auto targetArea = topRowArea.removeFromLeft(halfWidth).reduced(2, 0);
            owner.modEnv2TargetLabel.setBounds(targetArea.removeFromTop(15));
            owner.modEnv2TargetCombo.setBounds(targetArea);
            
            // Mode dropdown (right half)
            auto modeArea = topRowArea.reduced(2, 0);
            owner.modEnv2ModeLabel.setBounds(modeArea.removeFromTop(15));
            owner.modEnv2ModeCombo.setBounds(modeArea);
            
            // Bottom row: 6 knobs horizontally (A, D, S, R, Amount, Rate)
            auto knobWidth = bottomRow.getWidth() / 6;
            
            const int minKnobSize = 30;  // Smaller knobs to fit 6 in row
            const int minLabelHeight = 12;
            const int minValueHeight = 12;
            
            // Attack knob
            auto attackArea = bottomRow.removeFromLeft(knobWidth).reduced(2);
            owner.modEnv2AttackLabel.setBounds(attackArea.removeFromTop(minLabelHeight));
            auto attackValueArea = attackArea.removeFromBottom(minValueHeight);
            auto attackKnobArea = attackArea.reduced(0, 1);
            auto attackKnobBounds = attackKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 4, attackKnobArea.getWidth()),
                juce::jlimit(minKnobSize, attackKnobArea.getHeight(), attackKnobArea.getHeight())
            );
            owner.modEnv2AttackSlider.setBounds(attackKnobBounds);
            owner.modEnv2AttackValue.setBounds(attackValueArea);
            
            // Decay knob
            auto decayArea = bottomRow.removeFromLeft(knobWidth).reduced(2);
            owner.modEnv2DecayLabel.setBounds(decayArea.removeFromTop(minLabelHeight));
            auto decayValueArea = decayArea.removeFromBottom(minValueHeight);
            auto decayKnobArea = decayArea.reduced(0, 1);
            auto decayKnobBounds = decayKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 4, decayKnobArea.getWidth()),
                juce::jlimit(minKnobSize, decayKnobArea.getHeight(), decayKnobArea.getHeight())
            );
            owner.modEnv2DecaySlider.setBounds(decayKnobBounds);
            owner.modEnv2DecayValue.setBounds(decayValueArea);
            
            // Sustain knob
            auto sustainArea = bottomRow.removeFromLeft(knobWidth).reduced(2);
            owner.modEnv2SustainLabel.setBounds(sustainArea.removeFromTop(minLabelHeight));
            auto sustainValueArea = sustainArea.removeFromBottom(minValueHeight);
            auto sustainKnobArea = sustainArea.reduced(0, 1);
            auto sustainKnobBounds = sustainKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 4, sustainKnobArea.getWidth()),
                juce::jlimit(minKnobSize, sustainKnobArea.getHeight(), sustainKnobArea.getHeight())
            );
            owner.modEnv2SustainSlider.setBounds(sustainKnobBounds);
            owner.modEnv2SustainValue.setBounds(sustainValueArea);
            
            // Release knob
            auto releaseArea = bottomRow.removeFromLeft(knobWidth).reduced(2);
            owner.modEnv2ReleaseLabel.setBounds(releaseArea.removeFromTop(minLabelHeight));
            auto releaseValueArea = releaseArea.removeFromBottom(minValueHeight);
            auto releaseKnobArea = releaseArea.reduced(0, 1);
            auto releaseKnobBounds = releaseKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 4, releaseKnobArea.getWidth()),
                juce::jlimit(minKnobSize, releaseKnobArea.getHeight(), releaseKnobArea.getHeight())
            );
            owner.modEnv2ReleaseSlider.setBounds(releaseKnobBounds);
            owner.modEnv2ReleaseValue.setBounds(releaseValueArea);
            
            // Amount knob
            auto amountArea = bottomRow.removeFromLeft(knobWidth).reduced(2);
            owner.modEnv2AmountLabel.setBounds(amountArea.removeFromTop(minLabelHeight));
            auto amountValueArea = amountArea.removeFromBottom(minValueHeight);
            auto amountKnobArea = amountArea.reduced(0, 1);
            auto amountKnobBounds = amountKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 4, amountKnobArea.getWidth()),
                juce::jlimit(minKnobSize, amountKnobArea.getHeight(), amountKnobArea.getHeight())
            );
            owner.modEnv2AmountSlider.setBounds(amountKnobBounds);
            owner.modEnv2AmountValue.setBounds(amountValueArea);
            
            // Rate knob
            auto rateArea = bottomRow.reduced(2);
            owner.modEnv2RateLabel.setBounds(rateArea.removeFromTop(minLabelHeight));
            auto rateValueArea = rateArea.removeFromBottom(minValueHeight);
            auto rateKnobArea = rateArea.reduced(0, 1);
            auto rateKnobBounds = rateKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 4, rateKnobArea.getWidth()),
                juce::jlimit(minKnobSize, rateKnobArea.getHeight(), rateKnobArea.getHeight())
            );
            owner.modEnv2RateSlider.setBounds(rateKnobBounds);
            owner.modEnv2RateValue.setBounds(rateValueArea);
        }

        // No custom paint needed - GroupComponents handle their own styling

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(5);

            // Split into 2x2 grid: LFO, FM (top row), ModEnv1, ModEnv2 (bottom row)
            auto halfWidth = bounds.getWidth() / 2;
            auto halfHeight = bounds.getHeight() / 2;
            
            auto topRow = bounds.removeFromTop(halfHeight).reduced(3);
            auto bottomRow = bounds.reduced(3);
            
            auto lfoArea = topRow.removeFromLeft(halfWidth).reduced(3);
            auto pmArea = topRow.reduced(3);  // Remaining right half of top row
            
            auto modEnv1Area = bottomRow.removeFromLeft(halfWidth).reduced(3);
            auto modEnv2Area = bottomRow.reduced(3);  // Remaining right half of bottom row

            // Layout components using GroupComponent positioning
            layoutGroupLFO(lfoArea);
            layoutGroupPM(pmArea);
            layoutGroupModEnv1(modEnv1Area);
            layoutGroupModEnv2(modEnv2Area);
        }

    private:
        FreOscEditor& owner;
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
            // GROUP LAYOUT: Setup effects routing and effect components with GroupComponent
            setupGroupEffectsRouting();
            setupGroupReverb();
            setupGroupDelay();
            setupGroupWavefolder();
        }

        void setupGroupEffectsRouting()
        {
            // GROUP SETUP: Add effects routing components to GroupComponent
            owner.applyComponentStyling(owner.effectsRoutingGroup);
            owner.effectsRoutingGroup.setText("Routing");
            
            // Setup routing components
            owner.applyComponentStyling(owner.effectsRoutingCombo);
            
            // Add components to the routing group (no label)
            owner.effectsRoutingGroup.addAndMakeVisible(owner.effectsRoutingCombo);
            
            // Add the group to the tab
            addAndMakeVisible(owner.effectsRoutingGroup);
        }

        void setupGroupReverb()
        {
            // GROUP SETUP: Add plate reverb components to GroupComponent
            owner.applyComponentStyling(owner.reverbGroup);
            owner.reverbGroup.setText("Plate Reverb");

            // Set names for plate reverb sliders so they can be identified as rotary knobs
            owner.platePreDelaySlider.setName("plate_predelay");
            owner.plateSizeSlider.setName("plate_size");
            owner.plateDampingSlider.setName("plate_damping");
            owner.plateDiffusionSlider.setName("plate_diffusion");
            owner.plateWetSlider.setName("plate_wet");
            owner.plateWidthSlider.setName("plate_width");

            // Setup components with proper styling (rotary knobs)
            owner.applyComponentStyling(owner.platePreDelaySlider);
            owner.applyComponentStyling(owner.plateSizeSlider);
            owner.applyComponentStyling(owner.plateDampingSlider);
            owner.applyComponentStyling(owner.plateDiffusionSlider);
            owner.applyComponentStyling(owner.plateWetSlider);
            owner.applyComponentStyling(owner.plateWidthSlider);

            owner.applyComponentStyling(owner.platePreDelayLabel);
            owner.applyComponentStyling(owner.plateSizeLabel);
            owner.applyComponentStyling(owner.plateDampingLabel);
            owner.applyComponentStyling(owner.plateDiffusionLabel);
            owner.applyComponentStyling(owner.plateWetLabel);
            owner.applyComponentStyling(owner.plateWidthLabel);
            
            owner.applyComponentStyling(owner.platePreDelayValue);
            owner.applyComponentStyling(owner.plateSizeValue);
            owner.applyComponentStyling(owner.plateDampingValue);
            owner.applyComponentStyling(owner.plateDiffusionValue);
            owner.applyComponentStyling(owner.plateWetValue);
            owner.applyComponentStyling(owner.plateWidthValue);

            // Set value label styling for plate reverb controls (white text like other value labels)
            auto setupValueLabel = [&](juce::Label& label) {
                label.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
                label.setJustificationType(juce::Justification::centred);
                label.setColour(juce::Label::textColourId, juce::Colours::white);
                label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            };

            setupValueLabel(owner.platePreDelayValue);
            setupValueLabel(owner.plateSizeValue);
            setupValueLabel(owner.plateDampingValue);
            setupValueLabel(owner.plateDiffusionValue);
            setupValueLabel(owner.plateWetValue);
            setupValueLabel(owner.plateWidthValue);

            // Set labels
            owner.platePreDelayLabel.setText("Pre-Delay", juce::dontSendNotification);
            owner.plateSizeLabel.setText("Size", juce::dontSendNotification);
            owner.plateDampingLabel.setText("Damping", juce::dontSendNotification);
            owner.plateDiffusionLabel.setText("Diffusion", juce::dontSendNotification);
            owner.plateWetLabel.setText("Wet Level", juce::dontSendNotification);
            owner.plateWidthLabel.setText("Width", juce::dontSendNotification);

            // Add components to the group
            owner.reverbGroup.addAndMakeVisible(owner.platePreDelayLabel);
            owner.reverbGroup.addAndMakeVisible(owner.platePreDelaySlider);
            owner.reverbGroup.addAndMakeVisible(owner.platePreDelayValue);
            
            owner.reverbGroup.addAndMakeVisible(owner.plateSizeLabel);
            owner.reverbGroup.addAndMakeVisible(owner.plateSizeSlider);
            owner.reverbGroup.addAndMakeVisible(owner.plateSizeValue);
            
            owner.reverbGroup.addAndMakeVisible(owner.plateDampingLabel);
            owner.reverbGroup.addAndMakeVisible(owner.plateDampingSlider);
            owner.reverbGroup.addAndMakeVisible(owner.plateDampingValue);
            
            owner.reverbGroup.addAndMakeVisible(owner.plateDiffusionLabel);
            owner.reverbGroup.addAndMakeVisible(owner.plateDiffusionSlider);
            owner.reverbGroup.addAndMakeVisible(owner.plateDiffusionValue);
            
            owner.reverbGroup.addAndMakeVisible(owner.plateWetLabel);
            owner.reverbGroup.addAndMakeVisible(owner.plateWetSlider);
            owner.reverbGroup.addAndMakeVisible(owner.plateWetValue);
            
            owner.reverbGroup.addAndMakeVisible(owner.plateWidthLabel);
            owner.reverbGroup.addAndMakeVisible(owner.plateWidthSlider);
            owner.reverbGroup.addAndMakeVisible(owner.plateWidthValue);

            // Add the group to the tab
            addAndMakeVisible(owner.reverbGroup);
        }

        void setupGroupDelay()
        {
            // GROUP SETUP: Add tape delay components to GroupComponent
            owner.applyComponentStyling(owner.delayGroup);
            owner.delayGroup.setText("Tape Delay");

            // Set names for tape delay sliders so they can be identified as rotary knobs
            owner.tapeTimeSlider.setName("tape_time");
            owner.tapeFeedbackSlider.setName("tape_feedback");
            owner.tapeToneSlider.setName("tape_tone");
            owner.tapeFlutterSlider.setName("tape_flutter");
            owner.tapeWetSlider.setName("tape_wet");
            owner.tapeWidthSlider.setName("tape_width");

            // Setup components with proper styling (rotary knobs)
            owner.applyComponentStyling(owner.tapeTimeSlider);
            owner.applyComponentStyling(owner.tapeFeedbackSlider);
            owner.applyComponentStyling(owner.tapeToneSlider);
            owner.applyComponentStyling(owner.tapeFlutterSlider);
            owner.applyComponentStyling(owner.tapeWetSlider);
            owner.applyComponentStyling(owner.tapeWidthSlider);

            owner.applyComponentStyling(owner.tapeTimeLabel);
            owner.applyComponentStyling(owner.tapeFeedbackLabel);
            owner.applyComponentStyling(owner.tapeToneLabel);
            owner.applyComponentStyling(owner.tapeFlutterLabel);
            owner.applyComponentStyling(owner.tapeWetLabel);
            owner.applyComponentStyling(owner.tapeWidthLabel);
            
            owner.applyComponentStyling(owner.tapeTimeValue);
            owner.applyComponentStyling(owner.tapeFeedbackValue);
            owner.applyComponentStyling(owner.tapeToneValue);
            owner.applyComponentStyling(owner.tapeFlutterValue);
            owner.applyComponentStyling(owner.tapeWetValue);
            owner.applyComponentStyling(owner.tapeWidthValue);

            // Set value label styling for tape delay controls (white text like other value labels)
            auto setupValueLabel = [&](juce::Label& label) {
                label.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
                label.setJustificationType(juce::Justification::centred);
                label.setColour(juce::Label::textColourId, juce::Colours::white);
                label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            };

            setupValueLabel(owner.tapeTimeValue);
            setupValueLabel(owner.tapeFeedbackValue);
            setupValueLabel(owner.tapeToneValue);
            setupValueLabel(owner.tapeFlutterValue);
            setupValueLabel(owner.tapeWetValue);
            setupValueLabel(owner.tapeWidthValue);

            // Set labels
            owner.tapeTimeLabel.setText("Time", juce::dontSendNotification);
            owner.tapeFeedbackLabel.setText("Feedback", juce::dontSendNotification);
            owner.tapeToneLabel.setText("Tone", juce::dontSendNotification);
            owner.tapeFlutterLabel.setText("Flutter", juce::dontSendNotification);
            owner.tapeWetLabel.setText("Wet Level", juce::dontSendNotification);
            owner.tapeWidthLabel.setText("Width", juce::dontSendNotification);

            // Add components to the group
            owner.delayGroup.addAndMakeVisible(owner.tapeTimeLabel);
            owner.delayGroup.addAndMakeVisible(owner.tapeTimeSlider);
            owner.delayGroup.addAndMakeVisible(owner.tapeTimeValue);
            
            owner.delayGroup.addAndMakeVisible(owner.tapeFeedbackLabel);
            owner.delayGroup.addAndMakeVisible(owner.tapeFeedbackSlider);
            owner.delayGroup.addAndMakeVisible(owner.tapeFeedbackValue);
            
            owner.delayGroup.addAndMakeVisible(owner.tapeToneLabel);
            owner.delayGroup.addAndMakeVisible(owner.tapeToneSlider);
            owner.delayGroup.addAndMakeVisible(owner.tapeToneValue);
            
            owner.delayGroup.addAndMakeVisible(owner.tapeFlutterLabel);
            owner.delayGroup.addAndMakeVisible(owner.tapeFlutterSlider);
            owner.delayGroup.addAndMakeVisible(owner.tapeFlutterValue);
            
            owner.delayGroup.addAndMakeVisible(owner.tapeWetLabel);
            owner.delayGroup.addAndMakeVisible(owner.tapeWetSlider);
            owner.delayGroup.addAndMakeVisible(owner.tapeWetValue);
            
            owner.delayGroup.addAndMakeVisible(owner.tapeWidthLabel);
            owner.delayGroup.addAndMakeVisible(owner.tapeWidthSlider);
            owner.delayGroup.addAndMakeVisible(owner.tapeWidthValue);

            // Add the group to the tab
            addAndMakeVisible(owner.delayGroup);
        }

        void setupGroupWavefolder()
        {
            // GROUP SETUP: Add wavefolder components to GroupComponent
            owner.applyComponentStyling(owner.wavefolderGroup);
            owner.wavefolderGroup.setText("Wavefolder Distortion");

            // Set names for wavefolder sliders so they can be identified as rotary knobs
            owner.wavefolderDriveSlider.setName("wavefolder_drive");
            owner.wavefolderThresholdSlider.setName("wavefolder_threshold");
            owner.wavefolderSymmetrySlider.setName("wavefolder_symmetry");
            owner.wavefolderMixSlider.setName("wavefolder_mix");
            owner.wavefolderOutputSlider.setName("wavefolder_output");

            // Setup components with proper styling (rotary knobs)
            owner.applyComponentStyling(owner.wavefolderDriveSlider);
            owner.applyComponentStyling(owner.wavefolderThresholdSlider);
            owner.applyComponentStyling(owner.wavefolderSymmetrySlider);
            owner.applyComponentStyling(owner.wavefolderMixSlider);
            owner.applyComponentStyling(owner.wavefolderOutputSlider);

            owner.applyComponentStyling(owner.wavefolderDriveLabel);
            owner.applyComponentStyling(owner.wavefolderThresholdLabel);
            owner.applyComponentStyling(owner.wavefolderSymmetryLabel);
            owner.applyComponentStyling(owner.wavefolderMixLabel);
            owner.applyComponentStyling(owner.wavefolderOutputLabel);
            
            owner.applyComponentStyling(owner.wavefolderDriveValue);
            owner.applyComponentStyling(owner.wavefolderThresholdValue);
            owner.applyComponentStyling(owner.wavefolderSymmetryValue);
            owner.applyComponentStyling(owner.wavefolderMixValue);
            owner.applyComponentStyling(owner.wavefolderOutputValue);

            // Set value label styling for wavefolder controls (white text like other value labels)
            auto setupValueLabel = [&](juce::Label& label) {
                label.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
                label.setJustificationType(juce::Justification::centred);
                label.setColour(juce::Label::textColourId, juce::Colours::white);
                label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            };

            setupValueLabel(owner.wavefolderDriveValue);
            setupValueLabel(owner.wavefolderThresholdValue);
            setupValueLabel(owner.wavefolderSymmetryValue);
            setupValueLabel(owner.wavefolderMixValue);
            setupValueLabel(owner.wavefolderOutputValue);

            // Set labels
            owner.wavefolderDriveLabel.setText("Drive", juce::dontSendNotification);
            owner.wavefolderThresholdLabel.setText("Threshold", juce::dontSendNotification);
            owner.wavefolderSymmetryLabel.setText("Symmetry", juce::dontSendNotification);
            owner.wavefolderMixLabel.setText("Mix", juce::dontSendNotification);
            owner.wavefolderOutputLabel.setText("Output", juce::dontSendNotification);

            // Add components to the group
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderDriveLabel);
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderDriveSlider);
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderDriveValue);
            
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderThresholdLabel);
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderThresholdSlider);
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderThresholdValue);
            
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderSymmetryLabel);
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderSymmetrySlider);
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderSymmetryValue);
            
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderMixLabel);
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderMixSlider);
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderMixValue);
            
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderOutputLabel);
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderOutputSlider);
            owner.wavefolderGroup.addAndMakeVisible(owner.wavefolderOutputValue);

            // Add the group to the tab
            addAndMakeVisible(owner.wavefolderGroup);
        }

        void layoutGroupEffectsRouting(juce::Rectangle<int> area)
        {
            // Layout effects routing using GroupComponent (matching filter routing)
            owner.effectsRoutingGroup.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = owner.effectsRoutingGroup.getLocalBounds().reduced(8);
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight));
            
            // Simple layout: combo only (no label)
            owner.effectsRoutingCombo.setBounds(bounds.reduced(2));
        }

        void layoutGroupReverb(juce::Rectangle<int> area)
        {
            // Layout plate reverb using GroupComponent
            owner.reverbGroup.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = owner.reverbGroup.getLocalBounds().reduced(8);
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight));
            
            // Six knobs in 2 rows of 3: Top: Pre-Delay, Size, Damping | Bottom: Diffusion, Wet Level, Width
            auto knobWidth = bounds.getWidth() / 3;
            auto rowHeight = bounds.getHeight() / 2;
            
            const int minKnobSize = 40;
            const int minLabelHeight = 12;
            const int minValueHeight = 12;
            
            // Top row: Pre-Delay, Size, Damping
            auto topRow = bounds.removeFromTop(rowHeight);
            
            // Pre-Delay knob (top-left)
            auto preDelayArea = topRow.removeFromLeft(knobWidth).reduced(3);
            owner.platePreDelayLabel.setBounds(preDelayArea.removeFromTop(minLabelHeight));
            auto preDelayValueArea = preDelayArea.removeFromBottom(minValueHeight);
            auto preDelayKnobArea = preDelayArea.reduced(0, 2);
            auto preDelayKnobBounds = preDelayKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, preDelayKnobArea.getWidth()),
                juce::jlimit(minKnobSize, preDelayKnobArea.getHeight(), preDelayKnobArea.getHeight())
            );
            owner.platePreDelaySlider.setBounds(preDelayKnobBounds);
            owner.platePreDelayValue.setBounds(preDelayValueArea);
            
            // Size knob (top-center)
            auto sizeArea = topRow.removeFromLeft(knobWidth).reduced(3);
            owner.plateSizeLabel.setBounds(sizeArea.removeFromTop(minLabelHeight));
            auto sizeValueArea = sizeArea.removeFromBottom(minValueHeight);
            auto sizeKnobArea = sizeArea.reduced(0, 2);
            auto sizeKnobBounds = sizeKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, sizeKnobArea.getWidth()),
                juce::jlimit(minKnobSize, sizeKnobArea.getHeight(), sizeKnobArea.getHeight())
            );
            owner.plateSizeSlider.setBounds(sizeKnobBounds);
            owner.plateSizeValue.setBounds(sizeValueArea);
            
            // Damping knob (top-right)
            auto dampingArea = topRow.reduced(3);
            owner.plateDampingLabel.setBounds(dampingArea.removeFromTop(minLabelHeight));
            auto dampingValueArea = dampingArea.removeFromBottom(minValueHeight);
            auto dampingKnobArea = dampingArea.reduced(0, 2);
            auto dampingKnobBounds = dampingKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, dampingKnobArea.getWidth()),
                juce::jlimit(minKnobSize, dampingKnobArea.getHeight(), dampingKnobArea.getHeight())
            );
            owner.plateDampingSlider.setBounds(dampingKnobBounds);
            owner.plateDampingValue.setBounds(dampingValueArea);
            
            // Bottom row: Diffusion, Wet Level, Width
            auto bottomRow = bounds;
            
            // Diffusion knob (bottom-left)
            auto diffusionArea = bottomRow.removeFromLeft(knobWidth).reduced(3);
            owner.plateDiffusionLabel.setBounds(diffusionArea.removeFromTop(minLabelHeight));
            auto diffusionValueArea = diffusionArea.removeFromBottom(minValueHeight);
            auto diffusionKnobArea = diffusionArea.reduced(0, 2);
            auto diffusionKnobBounds = diffusionKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, diffusionKnobArea.getWidth()),
                juce::jlimit(minKnobSize, diffusionKnobArea.getHeight(), diffusionKnobArea.getHeight())
            );
            owner.plateDiffusionSlider.setBounds(diffusionKnobBounds);
            owner.plateDiffusionValue.setBounds(diffusionValueArea);
            
            // Wet Level knob (bottom-center)
            auto wetArea = bottomRow.removeFromLeft(knobWidth).reduced(3);
            owner.plateWetLabel.setBounds(wetArea.removeFromTop(minLabelHeight));
            auto wetValueArea = wetArea.removeFromBottom(minValueHeight);
            auto wetKnobArea = wetArea.reduced(0, 2);
            auto wetKnobBounds = wetKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, wetKnobArea.getWidth()),
                juce::jlimit(minKnobSize, wetKnobArea.getHeight(), wetKnobArea.getHeight())
            );
            owner.plateWetSlider.setBounds(wetKnobBounds);
            owner.plateWetValue.setBounds(wetValueArea);
            
            // Width knob (bottom-right)
            auto widthArea = bottomRow.reduced(3);
            owner.plateWidthLabel.setBounds(widthArea.removeFromTop(minLabelHeight));
            auto widthValueArea = widthArea.removeFromBottom(minValueHeight);
            auto widthKnobArea = widthArea.reduced(0, 2);
            auto widthKnobBounds = widthKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, widthKnobArea.getWidth()),
                juce::jlimit(minKnobSize, widthKnobArea.getHeight(), widthKnobArea.getHeight())
            );
            owner.plateWidthSlider.setBounds(widthKnobBounds);
            owner.plateWidthValue.setBounds(widthValueArea);
        }
        
        void layoutGroupDelay(juce::Rectangle<int> area)
        {
            // Layout tape delay using GroupComponent
            owner.delayGroup.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = owner.delayGroup.getLocalBounds().reduced(8);
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight));
            
            // Six knobs in 2 rows of 3: Top: Time, Feedback, Tone | Bottom: Flutter, Wet Level, Width
            auto knobWidth = bounds.getWidth() / 3;
            auto rowHeight = bounds.getHeight() / 2;
            
            const int minKnobSize = 40;
            const int minLabelHeight = 12;
            const int minValueHeight = 12;
            
            // Top row: Time, Feedback, Tone
            auto topRow = bounds.removeFromTop(rowHeight);
            
            // Time knob (top-left)
            auto timeArea = topRow.removeFromLeft(knobWidth).reduced(3);
            owner.tapeTimeLabel.setBounds(timeArea.removeFromTop(minLabelHeight));
            auto timeValueArea = timeArea.removeFromBottom(minValueHeight);
            auto timeKnobArea = timeArea.reduced(0, 2);
            auto timeKnobBounds = timeKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, timeKnobArea.getWidth()),
                juce::jlimit(minKnobSize, timeKnobArea.getHeight(), timeKnobArea.getHeight())
            );
            owner.tapeTimeSlider.setBounds(timeKnobBounds);
            owner.tapeTimeValue.setBounds(timeValueArea);
            
            // Feedback knob (top-center)
            auto feedbackArea = topRow.removeFromLeft(knobWidth).reduced(3);
            owner.tapeFeedbackLabel.setBounds(feedbackArea.removeFromTop(minLabelHeight));
            auto feedbackValueArea = feedbackArea.removeFromBottom(minValueHeight);
            auto feedbackKnobArea = feedbackArea.reduced(0, 2);
            auto feedbackKnobBounds = feedbackKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, feedbackKnobArea.getWidth()),
                juce::jlimit(minKnobSize, feedbackKnobArea.getHeight(), feedbackKnobArea.getHeight())
            );
            owner.tapeFeedbackSlider.setBounds(feedbackKnobBounds);
            owner.tapeFeedbackValue.setBounds(feedbackValueArea);
            
            // Tone knob (top-right)
            auto toneArea = topRow.reduced(3);
            owner.tapeToneLabel.setBounds(toneArea.removeFromTop(minLabelHeight));
            auto toneValueArea = toneArea.removeFromBottom(minValueHeight);
            auto toneKnobArea = toneArea.reduced(0, 2);
            auto toneKnobBounds = toneKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, toneKnobArea.getWidth()),
                juce::jlimit(minKnobSize, toneKnobArea.getHeight(), toneKnobArea.getHeight())
            );
            owner.tapeToneSlider.setBounds(toneKnobBounds);
            owner.tapeToneValue.setBounds(toneValueArea);
            
            // Bottom row: Flutter, Wet Level, Width
            auto bottomRow = bounds;
            
            // Flutter knob (bottom-left)
            auto flutterArea = bottomRow.removeFromLeft(knobWidth).reduced(3);
            owner.tapeFlutterLabel.setBounds(flutterArea.removeFromTop(minLabelHeight));
            auto flutterValueArea = flutterArea.removeFromBottom(minValueHeight);
            auto flutterKnobArea = flutterArea.reduced(0, 2);
            auto flutterKnobBounds = flutterKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, flutterKnobArea.getWidth()),
                juce::jlimit(minKnobSize, flutterKnobArea.getHeight(), flutterKnobArea.getHeight())
            );
            owner.tapeFlutterSlider.setBounds(flutterKnobBounds);
            owner.tapeFlutterValue.setBounds(flutterValueArea);
            
            // Wet Level knob (bottom-center)
            auto wetArea = bottomRow.removeFromLeft(knobWidth).reduced(3);
            owner.tapeWetLabel.setBounds(wetArea.removeFromTop(minLabelHeight));
            auto wetValueArea = wetArea.removeFromBottom(minValueHeight);
            auto wetKnobArea = wetArea.reduced(0, 2);
            auto wetKnobBounds = wetKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, wetKnobArea.getWidth()),
                juce::jlimit(minKnobSize, wetKnobArea.getHeight(), wetKnobArea.getHeight())
            );
            owner.tapeWetSlider.setBounds(wetKnobBounds);
            owner.tapeWetValue.setBounds(wetValueArea);
            
            // Width knob (bottom-right)
            auto widthArea = bottomRow.reduced(3);
            owner.tapeWidthLabel.setBounds(widthArea.removeFromTop(minLabelHeight));
            auto widthValueArea = widthArea.removeFromBottom(minValueHeight);
            auto widthKnobArea = widthArea.reduced(0, 2);
            auto widthKnobBounds = widthKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, widthKnobArea.getWidth()),
                juce::jlimit(minKnobSize, widthKnobArea.getHeight(), widthKnobArea.getHeight())
            );
            owner.tapeWidthSlider.setBounds(widthKnobBounds);
            owner.tapeWidthValue.setBounds(widthValueArea);
        }

        void layoutGroupWavefolder(juce::Rectangle<int> area)
        {
            // Layout Wavefolder using GroupComponent similar to reverb/delay
            owner.wavefolderGroup.setBounds(area);
            
            // Layout components within the group - account for bottom band
            auto bounds = owner.wavefolderGroup.getLocalBounds().reduced(8);
            
            // Reserve space for the bottom colored band (text height + padding)
            juce::Font f(juce::FontOptions(14.0f).withStyle("Bold"));
            auto textH = f.getHeight();
            auto bottomBandHeight = textH + 8.0f;
            bounds.removeFromBottom(static_cast<int>(bottomBandHeight));
            
            // 5 knobs in a single row: Drive, Threshold, Symmetry, Mix, Output
            auto knobWidth = bounds.getWidth() / 5;
            const int minKnobSize = 40;
            const int minLabelHeight = 15;
            const int minValueHeight = 15;
            
            // Drive knob
            auto driveArea = bounds.removeFromLeft(knobWidth).reduced(3);
            owner.wavefolderDriveLabel.setBounds(driveArea.removeFromTop(minLabelHeight));
            auto driveValueArea = driveArea.removeFromBottom(minValueHeight);
            auto driveKnobArea = driveArea.reduced(0, 2);
            auto driveKnobBounds = driveKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, driveKnobArea.getWidth()),
                juce::jlimit(minKnobSize, driveKnobArea.getHeight(), driveKnobArea.getHeight())
            );
            owner.wavefolderDriveSlider.setBounds(driveKnobBounds);
            owner.wavefolderDriveValue.setBounds(driveValueArea);
            
            // Threshold knob
            auto thresholdArea = bounds.removeFromLeft(knobWidth).reduced(3);
            owner.wavefolderThresholdLabel.setBounds(thresholdArea.removeFromTop(minLabelHeight));
            auto thresholdValueArea = thresholdArea.removeFromBottom(minValueHeight);
            auto thresholdKnobArea = thresholdArea.reduced(0, 2);
            auto thresholdKnobBounds = thresholdKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, thresholdKnobArea.getWidth()),
                juce::jlimit(minKnobSize, thresholdKnobArea.getHeight(), thresholdKnobArea.getHeight())
            );
            owner.wavefolderThresholdSlider.setBounds(thresholdKnobBounds);
            owner.wavefolderThresholdValue.setBounds(thresholdValueArea);
            
            // Symmetry knob
            auto symmetryArea = bounds.removeFromLeft(knobWidth).reduced(3);
            owner.wavefolderSymmetryLabel.setBounds(symmetryArea.removeFromTop(minLabelHeight));
            auto symmetryValueArea = symmetryArea.removeFromBottom(minValueHeight);
            auto symmetryKnobArea = symmetryArea.reduced(0, 2);
            auto symmetryKnobBounds = symmetryKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, symmetryKnobArea.getWidth()),
                juce::jlimit(minKnobSize, symmetryKnobArea.getHeight(), symmetryKnobArea.getHeight())
            );
            owner.wavefolderSymmetrySlider.setBounds(symmetryKnobBounds);
            owner.wavefolderSymmetryValue.setBounds(symmetryValueArea);
            
            // Mix knob
            auto mixArea = bounds.removeFromLeft(knobWidth).reduced(3);
            owner.wavefolderMixLabel.setBounds(mixArea.removeFromTop(minLabelHeight));
            auto mixValueArea = mixArea.removeFromBottom(minValueHeight);
            auto mixKnobArea = mixArea.reduced(0, 2);
            auto mixKnobBounds = mixKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, mixKnobArea.getWidth()),
                juce::jlimit(minKnobSize, mixKnobArea.getHeight(), mixKnobArea.getHeight())
            );
            owner.wavefolderMixSlider.setBounds(mixKnobBounds);
            owner.wavefolderMixValue.setBounds(mixValueArea);
            
            // Output knob
            auto outputArea = bounds.reduced(3);
            owner.wavefolderOutputLabel.setBounds(outputArea.removeFromTop(minLabelHeight));
            auto outputValueArea = outputArea.removeFromBottom(minValueHeight);
            auto outputKnobArea = outputArea.reduced(0, 2);
            auto outputKnobBounds = outputKnobArea.withSizeKeepingCentre(
                juce::jlimit(minKnobSize, knobWidth - 8, outputKnobArea.getWidth()),
                juce::jlimit(minKnobSize, outputKnobArea.getHeight(), outputKnobArea.getHeight())
            );
            owner.wavefolderOutputSlider.setBounds(outputKnobBounds);
            owner.wavefolderOutputValue.setBounds(outputValueArea);
        }

        // No custom paint needed - GroupComponents handle their own styling

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(5);

            // Reserve space for effects routing at the top (matching filter routing)
            auto routingHeight = 80;  // Same size as filter routing
            auto routingArea = bounds.removeFromTop(routingHeight).reduced(3);

            // Arrange effects in 2x2 grid: wavefolder on top row (full width), reverb and delay on bottom row
            auto halfHeight = bounds.getHeight() / 2;
            auto topRow = bounds.removeFromTop(halfHeight);
            auto bottomRow = bounds;
            
            // Top row: wavefolder (full width)
            auto wavefolderArea = topRow.reduced(3);
            
            // Bottom row: reverb and delay side by side
            auto sectionWidth = bottomRow.getWidth() / 2;
            auto reverbArea = bottomRow.removeFromLeft(sectionWidth).reduced(3);
            auto delayArea = bottomRow.reduced(3);

            // Layout components using GroupComponent positioning
            layoutGroupEffectsRouting(routingArea);
            layoutGroupWavefolder(wavefolderArea);
            layoutGroupReverb(reverbArea);
            layoutGroupDelay(delayArea);
        }

    private:
        FreOscEditor& owner;
    };

    return std::make_unique<EffectsTabComponent>(*this);
}


//==============================================================================
// Tab-specific setup methods

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
    // Note: waveformSelector and octaveSelector are custom components - no setup needed
    section.octaveLabel.setText("Octave", juce::dontSendNotification);
    setupSliderForTab(section.levelSlider, section.levelLabel, section.levelValue, "Level", paramPrefix + "level");
    setupSliderForTab(section.detuneSlider, section.detuneLabel, section.detuneValue, "Detune", paramPrefix + "detune");
    setupSliderForTab(section.panSlider, section.panLabel, section.panValue, "Pan", paramPrefix + "pan");
}

void FreOscEditor::applyComponentStyling(juce::Component& component)
{
    // Apply styling without calling addAndMakeVisible
    // This is the same as setupComponent but without adding to main editor

    // Apply FL Studio 3x Osc inspired styling - metallic grey look
    if (auto* label = dynamic_cast<juce::Label*>(&component))
    {
        label->setColour(juce::Label::textColourId, juce::Colours::white); // Changed to white
        label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label->setJustificationType(juce::Justification::centred);
    }
    else if (auto* slider = dynamic_cast<juce::Slider*>(&component))
    {
        // Check slider type by name
        juce::String sliderName = slider->getName();
        bool isRotaryKnob = sliderName.contains("noise") || sliderName.contains("plate") || sliderName.contains("tape") || sliderName.contains("wavefolder");
        
        if (isRotaryKnob)
        {
            // Noise, plate reverb, tape delay, and wavefolder sliders are rotary knobs with LEDs
            slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            // Metallic knob styling like other rotary controls
            slider->setColour(juce::Slider::rotarySliderFillColourId, accentColour);
            slider->setColour(juce::Slider::rotarySliderOutlineColourId, knobColour.darker(0.3f));
            slider->setColour(juce::Slider::thumbColourId, juce::Colour(0xff87ceeb)); // Light blue thumb
            slider->setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
        }
        else
        {
            // Other sliders (oscillator, LFO) are vertical with LEDs
            slider->setSliderStyle(juce::Slider::LinearVertical);
            slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            // Vertical slider styling
            slider->setColour(juce::Slider::trackColourId, juce::Colour(0xff202020)); // Dark track like knob rim
            slider->setColour(juce::Slider::thumbColourId, juce::Colour(0xffc0c0c0)); // Metallic thumb like knob center
            slider->setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
        }
    }
    else if (auto* combo = dynamic_cast<juce::ComboBox*>(&component))
    {
        // Retro 80s black plastic dropdown styling (matching waveform/octave selectors)
        combo->setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a)); // Dark black plastic background
        combo->setColour(juce::ComboBox::textColourId, juce::Colour(0xffa0a0a0)); // Light grey text (unselected state)
        combo->setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff707070)); // Medium grey outline (raised look)
        combo->setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffa0a0a0)); // Light grey arrow
        combo->setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff2a2a2a)); // Same as background
        combo->setColour(juce::ComboBox::focusedOutlineColourId, juce::Colour(0xff909090)); // Slightly brighter when focused
        
        // Dropdown menu colors (popup window)
        combo->setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff2a2a2a)); // Dark popup background
        combo->setColour(juce::PopupMenu::textColourId, juce::Colour(0xffa0a0a0)); // Light grey text in popup
        combo->setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff404040)); // Highlighted item background
        combo->setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white); // White text when highlighted
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
        // GroupComponent colors now handled by LookAndFeel - just set text color
        group->setColour(juce::GroupComponent::textColourId, textColour);
    }
}