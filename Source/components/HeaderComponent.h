/*
  ==============================================================================

    HeaderComponent.h
    MOONRUNNER header: title, subline, separator, synthesis mode selector, status pill

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../ui/MoonrunnerStyle.h"

//==============================================================================
class HeaderComponent : public juce::Component,
                        public juce::Timer
{
public:
    explicit HeaderComponent (juce::AudioProcessorValueTreeState& apvts);
    ~HeaderComponent() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setIsActive (bool active);
    void setScale (float s) { scale = s; }

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;
    juce::ComboBox modeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;

    bool isActive = false;
    float scale = 1.0f;
    float pulseAlpha = 0.5f;
    float pulseDir = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderComponent)
};
