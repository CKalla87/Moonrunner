/*
  ==============================================================================

    EnvelopeComponent.cpp

  ==============================================================================
*/

#include "EnvelopeComponent.h"

//==============================================================================
EnvelopeComponent::EnvelopeComponent (juce::AudioProcessorValueTreeState& apvts_)
    : apvts (apvts_)
{
    for (auto* s : { &attackSlider, &decaySlider, &sustainSlider, &releaseSlider })
    {
        addAndMakeVisible (s);
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        // TEMP: Use default LAF to test if sliders respond - remove to restore custom look
        // s->setLookAndFeel (&laf);
    }

    attackAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "attack", attackSlider);
    decayAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "decay", decaySlider);
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "sustain", sustainSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "release", releaseSlider);
    startTimerHz (20);  // Refresh value labels
}

EnvelopeComponent::~EnvelopeComponent()
{
    // attackSlider.setLookAndFeel (nullptr);
    // decaySlider.setLookAndFeel (nullptr);
    // sustainSlider.setLookAndFeel (nullptr);
    // releaseSlider.setLookAndFeel (nullptr);
}

void EnvelopeComponent::timerCallback()
{
    repaint();
}

juce::String EnvelopeComponent::formatValue (const juce::String& paramId, float value)
{
    if (paramId == "sustain")
        return juce::String (value, 2);  // "0.70"
    return juce::String (value, 2) + "s";  // "0.10s", "0.30s", "0.50s"
}

void EnvelopeComponent::drawSliderRow (juce::Graphics& g, juce::Rectangle<float> area,
                                        const juce::String& label, const juce::String& valueText)
{
    float labelH = 16.0f * scale;
    float gap = 4.0f * scale;

    g.setFont (MoonrunnerStyle::getMonoFont (10.0f * scale, false));
    g.setColour (MoonrunnerStyle::neonCyan());
    g.drawText (label, area.toNearestInt(), juce::Justification::centredLeft, true);

    g.setColour (MoonrunnerStyle::neonPink());
    g.drawText (valueText, area.toNearestInt(), juce::Justification::centredRight, true);
}

void EnvelopeComponent::paint (juce::Graphics& g)
{
    const float outlineInset = 10.0f;
    auto fullBounds = getLocalBounds().toFloat();
    auto r = fullBounds.reduced (outlineInset);
    float pad = 6.0f * scale;

    // Panel header: ENVELOPE (pink) + dot
    float headerTextH = 18.0f * scale;
    float dotSize = 5.0f * scale;
    float dotY = r.getY() + pad + (headerTextH - dotSize) * 0.5f;
    g.setColour (MoonrunnerStyle::neonPink());
    g.setFont (MoonrunnerStyle::getMonoFont (11.0f * scale, true));
    g.fillEllipse (r.getX() + pad, dotY, dotSize, dotSize);
    g.drawText ("ENVELOPE", r.getX() + pad + dotSize + 6, r.getY() + pad,
                static_cast<int>(r.getWidth()), static_cast<int>(headerTextH),
                juce::Justification::centredLeft, true);

    // Draw labels and values for each row
    int headerH = static_cast<int>(32.0f * scale);
    float rowH = (r.getHeight() - headerH) / 4.0f;
    float labelH = 16.0f * scale;
    float gap = 4.0f * scale;
    float baseY = r.getY() + headerH;

    auto rowArea = [&](int row) {
        return juce::Rectangle<float> (r.getX() + pad, baseY + row * rowH, r.getWidth() - pad * 2, labelH + gap);
    };

    drawSliderRow (g, rowArea (0), "ATTACK",  formatValue ("attack",  static_cast<float>(attackSlider.getValue())));
    drawSliderRow (g, rowArea (1), "DECAY",   formatValue ("decay",   static_cast<float>(decaySlider.getValue())));
    drawSliderRow (g, rowArea (2), "SUSTAIN", formatValue ("sustain", static_cast<float>(sustainSlider.getValue())));
    drawSliderRow (g, rowArea (3), "RELEASE", formatValue ("release", static_cast<float>(releaseSlider.getValue())));

    // Panel outline (neon pink, smooth corners like main)
    float borderPx = MoonrunnerStyle::innerPanelBorderPx * scale;
    float cornerRadius = MoonrunnerStyle::innerPanelCornerRadius * scale;
    g.setColour (MoonrunnerStyle::neonPink());
    g.drawRoundedRectangle (fullBounds, cornerRadius, borderPx);
}

void EnvelopeComponent::resized()
{
    const int outlineInset = 10;
    auto r = getLocalBounds().reduced (outlineInset);
    int headerH = static_cast<int>(32.0f * scale);
    float rowH = static_cast<float>(r.getHeight() - headerH) / 4.0f;
    float labelH = 18.0f * scale;
    float gap = 6.0f * scale;
    int minSliderH = static_cast<int>(28.0f * scale);

    auto sliderArea = [&](int row) {
        int y = headerH + static_cast<int>(row * rowH) + static_cast<int>(labelH + gap);
        int h = juce::jmax (minSliderH, static_cast<int>(rowH - labelH - gap));
        return r.withY (y).withHeight (h);
    };

    attackSlider.setBounds  (sliderArea (0));
    decaySlider.setBounds   (sliderArea (1));
    sustainSlider.setBounds (sliderArea (2));
    releaseSlider.setBounds (sliderArea (3));
}
