/*
  ==============================================================================

    EnvelopeComponent.h
    ADSR: ATTACK, DECAY, SUSTAIN, RELEASE with custom sliders

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../ui/MoonrunnerStyle.h"
#include "../ui/MoonrunnerLookAndFeel.h"

//==============================================================================
class EnvelopeComponent : public juce::Component,
                          public juce::Timer
{
public:
    EnvelopeComponent (juce::AudioProcessorValueTreeState& apvts);
    ~EnvelopeComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setScale (float s)
    {
        scale = s;
        laf.setScale (s);
        // for (auto* sl : { &attackSlider, &decaySlider, &sustainSlider, &releaseSlider })
        //     sl->setLookAndFeel (&laf);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    MoonrunnerLookAndFeel laf;

    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;

    float scale = 1.0f;

    void drawSliderRow (juce::Graphics& g, juce::Rectangle<float> area,
                        const juce::String& label, const juce::String& valueText);
    void timerCallback() override;
    juce::String formatValue (const juce::String& paramId, float value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeComponent)
};
