/*
  ==============================================================================

    MoonrunnerLookAndFeel.cpp

  ==============================================================================
*/

#include "MoonrunnerLookAndFeel.h"

//==============================================================================
void MoonrunnerLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float minSliderPos, float maxSliderPos,
                                              const juce::Slider::SliderStyle, juce::Slider& slider)
{
    auto trackHeight = juce::jmax (4, static_cast<int>(4.0f * scale));
    auto thumbRadius = static_cast<int>(12.0f * scale);
    auto trackY = y + height / 2 - trackHeight / 2;
    // Inset track so thumb stays within bounds and doesn't get clipped
    auto trackRect = juce::Rectangle<int> (x + thumbRadius, trackY, width - thumbRadius * 2, trackHeight).toFloat();

    // Track background (dark)
    g.setColour (MoonrunnerStyle::panelFillBot().darker (0.5f));
    g.fillRoundedRectangle (trackRect, trackHeight * 0.5f);

    // Active track (gradient cyan -> pink, from left to thumb)
    float normPos = (maxSliderPos > minSliderPos)
        ? (sliderPos - minSliderPos) / (maxSliderPos - minSliderPos)
        : 0.0f;
    normPos = juce::jlimit (0.0f, 1.0f, normPos);

    auto activeWidth = trackRect.getWidth() * normPos;
    if (activeWidth > 1.0f)
    {
        auto activeRect = trackRect.withWidth (activeWidth);
        juce::ColourGradient grad (MoonrunnerStyle::neonCyan(), activeRect.getX(), activeRect.getY(),
                                   MoonrunnerStyle::neonPink(), activeRect.getRight(), activeRect.getY(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (activeRect, trackHeight * 0.5f);
    }

    // Thumb
    auto thumbCentreX = trackRect.getX() + trackRect.getWidth() * normPos;
    auto thumbCentreY = trackRect.getCentreY();
    auto thumbRect = juce::Rectangle<float> (thumbCentreX - thumbRadius, thumbCentreY - thumbRadius,
                                             thumbRadius * 2.0f, thumbRadius * 2.0f);

    // Thumb glow
    for (int i = 2; i >= 1; --i)
    {
        float expand = static_cast<float>(i) * 2.0f;
        g.setColour (MoonrunnerStyle::neonPink().withAlpha (0.3f / i));
        g.fillEllipse (thumbRect.expanded (expand));
    }

    // Thumb fill (radial gradient pink center -> cyan edge)
    juce::ColourGradient thumbGrad (MoonrunnerStyle::neonPink(), thumbRect.getCentreX(), thumbRect.getCentreY(),
                                    MoonrunnerStyle::neonCyan(), thumbRect.getRight(), thumbRect.getCentreY(), true);
    thumbGrad.addColour (0.5f, MoonrunnerStyle::pinkMid());
    g.setGradientFill (thumbGrad);
    g.fillEllipse (thumbRect);

    // Thumb border (white)
    g.setColour (juce::Colours::white);
    g.drawEllipse (thumbRect, 0.5f);
}

//==============================================================================
void MoonrunnerLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour&,
                                                  bool shouldDrawButtonAsHighlighted,
                                                  bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    bool isActive = button.getToggleState();

    const float btnRadius = 8.0f;
    const float btnStroke = 0.5f;

    if (isActive)
    {
        // Active: pink gradient fill
        juce::ColourGradient grad (MoonrunnerStyle::neonPink(), bounds.getX(), bounds.getY(),
                                   MoonrunnerStyle::pinkMid(), bounds.getRight(), bounds.getY(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, btnRadius);

        // Glow
        g.setColour (MoonrunnerStyle::neonPink().withAlpha (0.4f));
        g.drawRoundedRectangle (bounds.expanded (2.0f), btnRadius + 2.0f, btnStroke);

        g.setColour (MoonrunnerStyle::neonPink());
        g.drawRoundedRectangle (bounds, btnRadius, btnStroke);
    }
    else
    {
        // Inactive: dark bg, cyan border 30% alpha
        g.setColour (MoonrunnerStyle::panelFillBot().darker (0.3f));
        g.fillRoundedRectangle (bounds, btnRadius);

        auto borderCol = MoonrunnerStyle::neonCyan().withAlpha (0.3f);
        if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
            borderCol = MoonrunnerStyle::neonCyan().withAlpha (0.7f);
        g.setColour (borderCol);
        g.drawRoundedRectangle (bounds, btnRadius, btnStroke);
    }
}

//==============================================================================
void MoonrunnerLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    auto bounds = button.getLocalBounds();
    bool isActive = button.getToggleState();

    g.setFont (MoonrunnerStyle::getMonoFont (12.0f * scale, true));
    g.setColour (isActive ? juce::Colours::white
                          : MoonrunnerStyle::neonCyan().withAlpha (0.7f));
    g.drawText (button.getButtonText(), bounds, juce::Justification::centred, true);
}

//==============================================================================
juce::Slider::SliderLayout MoonrunnerLookAndFeel::getSliderLayout (juce::Slider& slider)
{
    auto layout = juce::LookAndFeel_V4::getSliderLayout (slider);
    layout.textBoxBounds = juce::Rectangle<int> (0, 0, 0, 0);  // No text box
    return layout;
}
