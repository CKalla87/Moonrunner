/*
  ==============================================================================

    FilterComponent.cpp

  ==============================================================================
*/

#include "FilterComponent.h"

//==============================================================================
FilterComponent::FilterComponent (juce::AudioProcessorValueTreeState& apvts_)
    : apvts (apvts_)
{
    addAndMakeVisible (cutoffSlider);
    addAndMakeVisible (resonanceSlider);

    cutoffSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    resonanceSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    cutoffSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    resonanceSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    // TEMP: Use default LAF to test - remove to restore custom look
    // cutoffSlider.setLookAndFeel (&laf);
    // resonanceSlider.setLookAndFeel (&laf);

    cutoffAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "cutoff", cutoffSlider);
    resonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "resonance", resonanceSlider);

    startTimerHz (30);
}

FilterComponent::~FilterComponent()
{
    // cutoffSlider.setLookAndFeel (nullptr);
    // resonanceSlider.setLookAndFeel (nullptr);
}

void FilterComponent::timerCallback()
{
    if (meterActive)
    {
        for (int i = 0; i < 20; ++i)
            meterHeights[i] = rng.nextFloat();
    }
    repaint();
}

void FilterComponent::drawMeter (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const int numBars = 20;
    float gap = 2.0f * scale;
    float barW = (bounds.getWidth() - (numBars - 1) * gap) / numBars;
    float maxH = bounds.getHeight();

    for (int i = 0; i < numBars; ++i)
    {
        float x = bounds.getX() + i * (barW + gap);
        float h;
        if (meterActive)
        {
            h = maxH * meterHeights[i];
            if (h < 4.0f) h = 4.0f;
            juce::ColourGradient grad (MoonrunnerStyle::neonPink(), x, bounds.getBottom(),
                                       MoonrunnerStyle::neonCyan(), x, bounds.getY(), false);
            g.setGradientFill (grad);
        }
        else
        {
            h = maxH * 0.1f;
            g.setColour (MoonrunnerStyle::neonCyan().withAlpha (0.2f));
        }
        g.fillRoundedRectangle (x, bounds.getBottom() - h, barW, h, 4.0f);
    }
}

void FilterComponent::paint (juce::Graphics& g)
{
    const float outlineInset = 10.0f;
    auto fullBounds = getLocalBounds().toFloat();
    auto r = fullBounds.reduced (outlineInset);
    float pad = 6.0f * scale;

    // Panel header: LOWPASS FILTER (pink) + dot
    float headerTextH = 18.0f * scale;
    float dotSize = 5.0f * scale;
    float dotY = r.getY() + pad + (headerTextH - dotSize) * 0.5f;
    g.setColour (MoonrunnerStyle::neonPink());
    g.setFont (MoonrunnerStyle::getMonoFont (11.0f * scale, true));
    g.fillEllipse (r.getX() + pad, dotY, dotSize, dotSize);
    g.drawText ("LOWPASS FILTER", r.getX() + pad + dotSize + 6, r.getY() + pad,
                static_cast<int>(r.getWidth()), static_cast<int>(headerTextH),
                juce::Justification::centredLeft, true);

    // Slider labels and values
    int headerH = static_cast<int>(32.0f * scale);
    float rowH = 48.0f * scale;
    float labelH = 16.0f * scale;
    float gap = 4.0f * scale;

    float y = r.getY() + headerH;

    g.setFont (MoonrunnerStyle::getMonoFont (10.0f * scale, false));
    g.setColour (MoonrunnerStyle::neonCyan());
    g.drawText ("CUTOFF FREQUENCY", r.getX() + pad, y, static_cast<int>(r.getWidth() - pad * 2), static_cast<int>(labelH),
                juce::Justification::centredLeft, true);
    g.setColour (MoonrunnerStyle::neonPink());
    g.drawText (juce::String (juce::roundToInt (cutoffSlider.getValue())) + " Hz",
                r.getX() + pad, y, static_cast<int>(r.getWidth() - pad * 2), static_cast<int>(labelH),
                juce::Justification::centredRight, true);

    y += rowH;
    g.setColour (MoonrunnerStyle::neonCyan());
    g.drawText ("RESONANCE", r.getX() + pad, y, static_cast<int>(r.getWidth() - pad * 2), static_cast<int>(labelH),
                juce::Justification::centredLeft, true);
    g.setColour (MoonrunnerStyle::neonPink());
    g.drawText (juce::String (resonanceSlider.getValue(), 1),
                r.getX() + pad, y, static_cast<int>(r.getWidth() - pad * 2), static_cast<int>(labelH),
                juce::Justification::centredRight, true);

    // Separator (cyan 20% alpha)
    float sepY = y + rowH + gap * 2;
    g.setColour (MoonrunnerStyle::neonCyan().withAlpha (0.2f));
    g.drawHorizontalLine (static_cast<int>(sepY), r.getX(), r.getRight());

    // Meter area
    float meterTop = sepY + gap * 4;
    auto meterBounds = r.withTrimmedTop (meterTop).reduced (pad, 0);
    drawMeter (g, meterBounds.toFloat());

    // Panel outline (neon pink, smooth corners like main)
    float borderPx = MoonrunnerStyle::innerPanelBorderPx * scale;
    float cornerRadius = MoonrunnerStyle::innerPanelCornerRadius * scale;
    g.setColour (MoonrunnerStyle::neonPink());
    g.drawRoundedRectangle (fullBounds, cornerRadius, borderPx);
}

void FilterComponent::resized()
{
    const int outlineInset = 10;
    auto r = getLocalBounds().reduced (outlineInset);
    int headerH = static_cast<int>(32.0f * scale);
    float rowH = 48.0f * scale;
    float labelH = 18.0f * scale;
    float gap = 6.0f * scale;
    int sliderH = juce::jmax (28, static_cast<int>(rowH - labelH - gap));

    cutoffSlider.setBounds (r.getX(), r.getY() + headerH + static_cast<int>(labelH + gap),
                            r.getWidth(), sliderH);
    resonanceSlider.setBounds (r.getX(), r.getY() + headerH + static_cast<int>(rowH + labelH + gap),
                               r.getWidth(), sliderH);
}
