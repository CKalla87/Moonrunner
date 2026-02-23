/*
  ==============================================================================

    HeaderComponent.cpp

  ==============================================================================
*/

#include "HeaderComponent.h"

//==============================================================================
HeaderComponent::HeaderComponent (juce::AudioProcessorValueTreeState& apvts_)
    : apvts (apvts_)
{
    startTimerHz (15);  // For ACTIVE pill pulse

    modeCombo.addItem ("FM", 1);
    modeCombo.addItem ("Analog", 2);
    modeCombo.addItem ("Sampler", 3);
    addAndMakeVisible (modeCombo);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, "SYNTHESIS_MODE", modeCombo);
}

void HeaderComponent::timerCallback()
{
    if (isActive)
    {
        pulseAlpha += pulseDir * 0.08f;
        if (pulseAlpha >= 1.0f) { pulseAlpha = 1.0f; pulseDir = -1.0f; }
        if (pulseAlpha <= 0.4f) { pulseAlpha = 0.4f; pulseDir = 1.0f; }
        repaint();
    }
}

void HeaderComponent::setIsActive (bool active)
{
    if (isActive != active)
    {
        isActive = active;
        repaint();
    }
}

void HeaderComponent::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    float pad = 4.0f * scale;

    // Title "MOONRUNNER" - gradient pink -> cyan, heavy monospace, wide tracking
    auto titleFont = MoonrunnerStyle::getMonoFont (42.0f * scale, true);
    titleFont.setExtraKerningFactor (0.15f);  // Wide tracking
    auto titleBounds = juce::Rectangle<int> (static_cast<int>(r.getX() + pad),
                                             static_cast<int>(r.getY() + pad),
                                             static_cast<int>(r.getWidth() * 0.6f),
                                             static_cast<int>(48.0f * scale));
    MoonrunnerStyle::drawGradientText (g, "MOONRUNNER", titleFont, titleBounds,
                                       MoonrunnerStyle::neonPink(), MoonrunnerStyle::neonCyan(), true);

    // Subline "SYNTH v2.0" - cyan, smaller, tracking
    auto subFont = MoonrunnerStyle::getMonoFont (12.0f * scale, false);
    subFont.setExtraKerningFactor (0.3f);
    g.setColour (MoonrunnerStyle::neonCyan());
    g.setFont (subFont);
    g.drawText ("SYNTH v2.0", titleBounds.getX(), titleBounds.getBottom() - 2,
                titleBounds.getWidth(), static_cast<int>(18.0f * scale),
                juce::Justification::centredLeft, true);

    // Separator line (cyan)
    float sepY = r.getY() + 70.0f * scale;
    g.setColour (MoonrunnerStyle::neonCyan().withAlpha (0.3f));
    g.drawLine (r.getX(), sepY, r.getRight(), sepY, 1.0f);

    // Status pill (top right)
    float pillW = 90.0f * scale;
    float pillH = 28.0f * scale;
    float pillX = r.getRight() - pillW - pad;
    float pillY = r.getY() + pad;
    auto pillRect = juce::Rectangle<float> (pillX, pillY, pillW, pillH);

    if (isActive)
    {
        g.setColour (MoonrunnerStyle::neonPink().withAlpha (0.2f));
        g.fillRoundedRectangle (pillRect, pillH * 0.5f);
        g.setColour (MoonrunnerStyle::neonPink());
        g.drawRoundedRectangle (pillRect, pillH * 0.5f, 1.0f);
        // Dot - pulsing
        float dotRadius = 4.0f * scale;
        g.setColour (MoonrunnerStyle::neonPink().withAlpha (pulseAlpha));
        g.fillEllipse (pillRect.getX() + 12.0f * scale - dotRadius,
                       pillRect.getCentreY() - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
        g.setColour (MoonrunnerStyle::neonCyan());  // ACTIVE text cyan to match React
        g.setFont (MoonrunnerStyle::getMonoFont (11.0f * scale, false));
        g.drawText ("ACTIVE", pillRect.toNearestInt(), juce::Justification::centred, true);
    }
    else
    {
        g.setColour (MoonrunnerStyle::neonCyan().withAlpha (0.1f));
        g.fillRoundedRectangle (pillRect, pillH * 0.5f);
        g.setColour (MoonrunnerStyle::neonCyan().withAlpha (0.4f));
        g.drawRoundedRectangle (pillRect, pillH * 0.5f, 1.0f);
        float dotRadius = 4.0f * scale;
        g.setColour (MoonrunnerStyle::neonCyan().withAlpha (0.5f));
        g.fillEllipse (pillRect.getX() + 12.0f * scale - dotRadius,
                       pillRect.getCentreY() - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
        g.setColour (MoonrunnerStyle::neonCyan());
        g.setFont (MoonrunnerStyle::getMonoFont (11.0f * scale, false));
        g.drawText ("IDLE", pillRect.toNearestInt(), juce::Justification::centred, true);
    }
}

void HeaderComponent::resized()
{
    auto r = getLocalBounds();
    float pad = 4.0f * scale;
    float modeW = 100.0f * scale;
    float modeH = 28.0f * scale;
    int pillW = 90;
    modeCombo.setBounds (r.getRight() - juce::roundToInt (modeW) - pillW - juce::roundToInt (pad * 2),
                         r.getY() + juce::roundToInt (pad),
                         juce::roundToInt (modeW), juce::roundToInt (modeH));
}
