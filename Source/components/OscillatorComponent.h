/*
  ==============================================================================

    OscillatorComponent.h
    2x2 button grid: SINE, SQUARE, SAWTOOTH, TRIANGLE

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../ui/MoonrunnerStyle.h"
#include "../ui/MoonrunnerLookAndFeel.h"

//==============================================================================
class OscillatorComponent : public juce::Component,
                            public juce::Timer
{
public:
    OscillatorComponent (juce::AudioProcessorValueTreeState& apvts);
    ~OscillatorComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setScale (float s) { scale = s; laf.setScale (s); }

private:
    juce::AudioProcessorValueTreeState& apvts;
    MoonrunnerLookAndFeel laf;

    juce::TextButton sineBtn;
    juce::TextButton squareBtn;
    juce::TextButton sawtoothBtn;
    juce::TextButton triangleBtn;

    float scale = 1.0f;

    void timerCallback() override;
    void setOscFromButton (int index);
    void updateButtonStates();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorComponent)
};
