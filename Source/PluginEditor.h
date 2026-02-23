/*
  ==============================================================================

    PluginEditor.h
    MOONRUNNER SYNTH v2.0 - Resizable editor with aspect-preserving scale

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/MoonrunnerStyle.h"
#include "components/HeaderComponent.h"
#include "components/OscillatorComponent.h"
#include "components/EnvelopeComponent.h"
#include "components/FilterComponent.h"
#include "components/KeyboardComponent.h"

//==============================================================================
class MoonrunnerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    MoonrunnerAudioProcessorEditor (MoonrunnerAudioProcessor&);
    ~MoonrunnerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Called from processor for external MIDI keyboard feedback
    void updateKeyboardState (int noteNumber, bool isNoteOn, float velocity);
    void clearKeyboardState();

private:
    MoonrunnerAudioProcessor& audioProcessor;

    HeaderComponent headerComponent;
    OscillatorComponent oscillatorComponent;
    EnvelopeComponent envelopeComponent;
    FilterComponent filterComponent;
    KeyboardComponent keyboardComponent;

    float uiScale = 1.0f;

    void updateActiveState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoonrunnerAudioProcessorEditor)
};
