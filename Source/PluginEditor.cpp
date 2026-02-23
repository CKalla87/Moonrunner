/*
  ==============================================================================

    PluginEditor.cpp
    MOONRUNNER SYNTH v2.0

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MoonrunnerAudioProcessorEditor::MoonrunnerAudioProcessorEditor (MoonrunnerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      headerComponent (*p.apvts),
      oscillatorComponent (*p.apvts),
      envelopeComponent (*p.apvts),
      filterComponent (*p.apvts)
{
    setOpaque (true);
    setSize (MoonrunnerStyle::baseWidth, MoonrunnerStyle::baseHeight);

    addAndMakeVisible (headerComponent);
    addAndMakeVisible (oscillatorComponent);
    addAndMakeVisible (envelopeComponent);
    addAndMakeVisible (filterComponent);
    addAndMakeVisible (keyboardComponent);

    keyboardComponent.onNote = [this](int note, float velocity, bool isOn)
    {
        juce::MidiMessage msg = isOn
            ? juce::MidiMessage::noteOn (1, note, velocity)
            : juce::MidiMessage::noteOff (1, note);
        msg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
        audioProcessor.processMidiMessage (msg);

        if (isOn)
            audioProcessor.activeNoteCount++;
        else
            audioProcessor.activeNoteCount = juce::jmax (0, audioProcessor.activeNoteCount.load() - 1);

        updateActiveState();
    };
}

MoonrunnerAudioProcessorEditor::~MoonrunnerAudioProcessorEditor()
{
    // Ensure host can request a new editor on reopen. Some hosts (e.g. Ableton) may destroy
    // the view without going through the normal teardown, leaving activeEditor set so
    // createEditorIfNeeded returns nullptr and the host caches "no UI" and never asks again.
    audioProcessor.editorBeingDeleted (this);
}

void MoonrunnerAudioProcessorEditor::updateKeyboardState (int noteNumber, bool isNoteOn, float velocity)
{
    juce::ignoreUnused (velocity);
    if (isNoteOn)
    {
        keyboardComponent.addActiveNote (noteNumber);
        audioProcessor.activeNoteCount++;
    }
    else
    {
        keyboardComponent.removeActiveNote (noteNumber);
        audioProcessor.activeNoteCount = juce::jmax (0, audioProcessor.activeNoteCount.load() - 1);
    }
    updateActiveState();
}

void MoonrunnerAudioProcessorEditor::clearKeyboardState()
{
    keyboardComponent.clearActiveNotes();
    audioProcessor.activeNoteCount = 0;
    updateActiveState();
}

void MoonrunnerAudioProcessorEditor::updateActiveState()
{
    bool active = audioProcessor.activeNoteCount.load() > 0;
    headerComponent.setIsActive (active);
    filterComponent.setMeterActive (active);
}

void MoonrunnerAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    uiScale = MoonrunnerStyle::getScale (bounds);

    // Background gradient
    juce::ColourGradient bgGrad (MoonrunnerStyle::bgDarkTop(), 0, 0,
                                 MoonrunnerStyle::bgDarkBot(), (float)bounds.getWidth(), (float)bounds.getHeight(), false);
    bgGrad.addColour (0.5f, MoonrunnerStyle::bgDarkMid());
    g.setGradientFill (bgGrad);
    g.fillAll();

    // Grid
    MoonrunnerStyle::drawGridBackground (g, bounds, uiScale);

    // Glow orbs
    MoonrunnerStyle::drawGlowOrb (g, juce::Point<float> (80.0f * uiScale, 80.0f * uiScale),
                                  120.0f * uiScale, MoonrunnerStyle::neonPink(), 1.2f);
    MoonrunnerStyle::drawGlowOrb (g, juce::Point<float> (bounds.getWidth() - 120.0f * uiScale,
                                                          bounds.getHeight() - 120.0f * uiScale),
                                  150.0f * uiScale, MoonrunnerStyle::neonCyan(), 1.2f);

    // Main synth panel
    float pad = 40.0f * uiScale;
    float cornerRadius = MoonrunnerStyle::panelCornerRadius * uiScale;
    float borderPx = MoonrunnerStyle::panelBorderPx * uiScale;
    auto panelRect = bounds.toFloat().reduced (pad);
    MoonrunnerStyle::drawNeonPanel (g, panelRect,
                                    MoonrunnerStyle::neonPink(), borderPx, cornerRadius,
                                    MoonrunnerStyle::panelFillTop(), MoonrunnerStyle::panelFillBot(), true);

    // Footer text (drawn over panel) - use drawn circles instead of Unicode bullets to avoid encoding issues
    g.setFont (MoonrunnerStyle::getMonoFont (14.0f * uiScale, false));
    g.setColour (MoonrunnerStyle::neonCyan().withAlpha (0.5f));
    const char* s1 = "DIGITAL WAVE SYNTHESIZER";
    const char* s2 = "CLICK KEYS TO PLAY";
    const char* s3 = "ADJUST PARAMETERS FOR SOUND DESIGN";
    const float gap = 12.0f * uiScale;
    const float dotRadius = 2.0f * uiScale;
    float w1 = g.getCurrentFont().getStringWidth (s1);
    float w2 = g.getCurrentFont().getStringWidth (s2);
    float w3 = g.getCurrentFont().getStringWidth (s3);
    float totalW = w1 + gap + dotRadius * 2 + gap + w2 + gap + dotRadius * 2 + gap + w3;
    auto footerArea = panelRect.withTrimmedBottom (20).toNearestInt();
    float x = (float) footerArea.getCentreX() - totalW * 0.5f;
    float lineH = 16.0f * uiScale;
    float baseY = (float) footerArea.getBottom() - 6.0f * uiScale;
    float textTop = baseY - lineH;
    float dotCentreY = baseY - lineH * 0.5f;
    g.drawText (s1, juce::roundToInt (x), juce::roundToInt (textTop),
                juce::roundToInt (w1), juce::roundToInt (lineH),
                juce::Justification::centredLeft, true);
    x += w1 + gap;
    g.fillEllipse (x, dotCentreY - dotRadius, dotRadius * 2, dotRadius * 2);
    x += dotRadius * 2 + gap;
    g.drawText (s2, juce::roundToInt (x), juce::roundToInt (textTop),
                juce::roundToInt (w2), juce::roundToInt (lineH),
                juce::Justification::centredLeft, true);
    x += w2 + gap;
    g.fillEllipse (x, dotCentreY - dotRadius, dotRadius * 2, dotRadius * 2);
    x += dotRadius * 2 + gap;
    g.drawText (s3, juce::roundToInt (x), juce::roundToInt (textTop),
                juce::roundToInt (w3), juce::roundToInt (lineH),
                juce::Justification::centredLeft, true);
}

void MoonrunnerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    uiScale = MoonrunnerStyle::getScale (bounds);

    float pad = 40.0f * uiScale;
    float innerPad = 24.0f * uiScale;
    auto panel = bounds.reduced ((int)pad);

    // Layout constants
    int headerH = static_cast<int>(90.0f * uiScale);
    int controlsH = static_cast<int>(380.0f * uiScale);
    int keyboardH = static_cast<int>(200.0f * uiScale);
    int footerH = static_cast<int>(40.0f * uiScale);

    int y = panel.getY() + (int)innerPad;

    // Header
    headerComponent.setScale (uiScale);
    headerComponent.setBounds (panel.getX() + (int)innerPad, y, panel.getWidth() - (int)(innerPad * 2), headerH);
    y += headerH + (int)(innerPad * 0.5f);

    // Controls: 2 columns
    int colW = (panel.getWidth() - (int)(innerPad * 3)) / 2;
    int leftX = panel.getX() + (int)innerPad;
    int rightX = leftX + colW + (int)innerPad;

    oscillatorComponent.setScale (uiScale);
    int oscH = static_cast<int>(120.0f * uiScale);
    oscillatorComponent.setBounds (leftX, y, colW, oscH);
    y += oscH + (int)(innerPad * 0.5f);

    envelopeComponent.setScale (uiScale);
    int envH = controlsH - oscH - (int)(innerPad * 0.5f);
    envelopeComponent.setBounds (leftX, y, colW, envH);

    filterComponent.setScale (uiScale);
    filterComponent.setBounds (rightX, panel.getY() + (int)innerPad + headerH + (int)(innerPad * 0.5f),
                               colW, controlsH);

    y = panel.getY() + headerH + controlsH + (int)innerPad;

    // Keyboard panel
    int kbPanelY = y;
    int kbPanelH = keyboardH + (int)(innerPad * 2);
    keyboardComponent.setScale (uiScale);
    keyboardComponent.setBounds (panel.getX() + (int)innerPad, kbPanelY + (int)innerPad,
                                 panel.getWidth() - (int)(innerPad * 2), keyboardH);

    updateActiveState();
}
