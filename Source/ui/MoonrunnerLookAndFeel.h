/*
  ==============================================================================

    MoonrunnerLookAndFeel.h
    Custom LookAndFeel for MOONRUNNER SYNTH v2.0 - sliders and buttons

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "MoonrunnerStyle.h"

//==============================================================================
class MoonrunnerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MoonrunnerLookAndFeel() = default;
    ~MoonrunnerLookAndFeel() override = default;

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    juce::Slider::SliderLayout getSliderLayout (juce::Slider& slider) override;

    int getSliderThumbRadius (juce::Slider&) override { return juce::jmax (1, static_cast<int>(12.0f * scale)); }

    void setScale (float s) { scale = s; }
    float getScale() const { return scale; }

private:
    float scale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoonrunnerLookAndFeel)
};
