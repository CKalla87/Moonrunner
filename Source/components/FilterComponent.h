/*
  ==============================================================================

    FilterComponent.h
    LOWPASS FILTER: cutoff, resonance, visual meter

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../ui/MoonrunnerStyle.h"
#include "../ui/MoonrunnerLookAndFeel.h"

//==============================================================================
class FilterComponent : public juce::Component,
                        public juce::Timer
{
public:
    FilterComponent (juce::AudioProcessorValueTreeState& apvts);
    ~FilterComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setScale (float s)
    {
        scale = s;
        laf.setScale (s);
        // cutoffSlider.setLookAndFeel (&laf);
        // resonanceSlider.setLookAndFeel (&laf);
    }
    void setMeterActive (bool active) { meterActive = active; }

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;
    MoonrunnerLookAndFeel laf;

    juce::Slider cutoffSlider;
    juce::Slider resonanceSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAttachment;

    float scale = 1.0f;
    bool meterActive = false;
    juce::Random rng;
    float meterHeights[20] = {};

    void drawMeter (juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterComponent)
};
