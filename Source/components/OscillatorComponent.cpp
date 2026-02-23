/*
  ==============================================================================

    OscillatorComponent.cpp

  ==============================================================================
*/

#include "OscillatorComponent.h"

//==============================================================================
OscillatorComponent::OscillatorComponent (juce::AudioProcessorValueTreeState& apvts_)
    : apvts (apvts_)
{
    for (auto* btn : { &sineBtn, &squareBtn, &sawtoothBtn, &triangleBtn })
    {
        addAndMakeVisible (btn);
        btn->setLookAndFeel (&laf);
        btn->setClickingTogglesState (true);
        btn->setRadioGroupId (1);
    }

    sineBtn.setButtonText ("SINE");
    squareBtn.setButtonText ("SQUARE");
    sawtoothBtn.setButtonText ("SAWTOOTH");
    triangleBtn.setButtonText ("TRIANGLE");

    sineBtn.onClick = [this]() { setOscFromButton (0); };
    squareBtn.onClick = [this]() { setOscFromButton (1); };
    sawtoothBtn.onClick = [this]() { setOscFromButton (2); };
    triangleBtn.onClick = [this]() { setOscFromButton (3); };

    // oscType is a Choice param - we manage it manually
    startTimerHz (10);  // Sync with param/automation
}

OscillatorComponent::~OscillatorComponent()
{
    sineBtn.setLookAndFeel (nullptr);
    squareBtn.setLookAndFeel (nullptr);
    sawtoothBtn.setLookAndFeel (nullptr);
    triangleBtn.setLookAndFeel (nullptr);
}

void OscillatorComponent::timerCallback()
{
    updateButtonStates();
}

void OscillatorComponent::setOscFromButton (int index)
{
    if (auto* param = apvts.getParameter ("oscType"))
    {
        // Choice: 4 items -> index 0..3 maps to normalized 0..1
        float norm = static_cast<float>(index) / 3.0f;
        param->setValueNotifyingHost (norm);
    }
    updateButtonStates();
}

void OscillatorComponent::updateButtonStates()
{
    int idx = 0;
    if (auto* param = apvts.getParameter ("oscType"))
    {
        float v = param->getValue();
        idx = juce::jlimit (0, 3, juce::roundToInt (v * 3.0f));
    }

    sineBtn.setToggleState    (idx == 0, juce::dontSendNotification);
    squareBtn.setToggleState  (idx == 1, juce::dontSendNotification);
    sawtoothBtn.setToggleState(idx == 2, juce::dontSendNotification);
    triangleBtn.setToggleState(idx == 3, juce::dontSendNotification);
}

void OscillatorComponent::paint (juce::Graphics& g)
{
    const float outlineInset = 10.0f;
    auto fullBounds = getLocalBounds().toFloat();
    auto r = fullBounds.reduced (outlineInset);
    float pad = 6.0f * scale;

    // Panel header: OSCILLATOR (cyan) + dot
    float headerTextH = 18.0f * scale;
    float dotSize = 5.0f * scale;
    float dotY = r.getY() + pad + (headerTextH - dotSize) * 0.5f;
    g.setColour (MoonrunnerStyle::neonCyan());
    g.setFont (MoonrunnerStyle::getMonoFont (11.0f * scale, true));
    g.fillEllipse (r.getX() + pad, dotY, dotSize, dotSize);
    g.drawText ("OSCILLATOR", r.getX() + pad + dotSize + 6, r.getY() + pad,
                static_cast<int>(r.getWidth()), static_cast<int>(headerTextH),
                juce::Justification::centredLeft, true);

    // Panel outline (light cyan, smooth corners like main)
    g.setColour (MoonrunnerStyle::neonCyan());
    g.drawRoundedRectangle (fullBounds, MoonrunnerStyle::innerPanelCornerRadius * scale,
                            MoonrunnerStyle::innerPanelBorderPx * scale);
}

void OscillatorComponent::resized()
{
    const int outlineInset = 10;
    auto r = getLocalBounds().reduced (outlineInset);
    int headerH = static_cast<int>(28.0f * scale);
    auto gridArea = r.withTrimmedTop (headerH);

    int gap = static_cast<int>(6.0f * scale);
    int btnW = (gridArea.getWidth() - gap) / 2;
    int btnH = (gridArea.getHeight() - gap) / 2;

    sineBtn.setBounds     (gridArea.getX(), gridArea.getY(), btnW, btnH);
    squareBtn.setBounds   (gridArea.getX() + btnW + gap, gridArea.getY(), btnW, btnH);
    sawtoothBtn.setBounds (gridArea.getX(), gridArea.getY() + btnH + gap, btnW, btnH);
    triangleBtn.setBounds (gridArea.getX() + btnW + gap, gridArea.getY() + btnH + gap, btnW, btnH);
}
