/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomMidiKeyboard.h"

//==============================================================================
/**
*/
class MoonrunnerAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                          public juce::MidiKeyboardStateListener
{
public:
    MoonrunnerAudioProcessorEditor (MoonrunnerAudioProcessor&);
    ~MoonrunnerAudioProcessorEditor() override;
    
    // MidiKeyboardStateListener
    void handleNoteOn (juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    
    // Methods to update keyboard state from external MIDI (called from processor)
    void updateKeyboardState (int noteNumber, bool isNoteOn, float velocity);
    void clearKeyboardState();

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MoonrunnerAudioProcessor& audioProcessor;

    // 80s synth color palette
    juce::Colour synthBlack = juce::Colour::fromFloatRGBA (0.1f, 0.1f, 0.15f, 1.0f);
    juce::Colour synthDark = juce::Colour::fromFloatRGBA (0.2f, 0.2f, 0.3f, 1.0f);
    juce::Colour synthCyan = juce::Colour::fromFloatRGBA (0.0f, 1.0f, 1.0f, 1.0f);
    juce::Colour synthMagenta = juce::Colour::fromFloatRGBA (1.0f, 0.0f, 1.0f, 1.0f);
    juce::Colour synthYellow = juce::Colour::fromFloatRGBA (1.0f, 1.0f, 0.0f, 1.0f);
    juce::Colour synthGreen = juce::Colour::fromFloatRGBA (0.0f, 1.0f, 0.5f, 1.0f);
    juce::Colour synthOrange = juce::Colour::fromFloatRGBA (1.0f, 0.5f, 0.0f, 1.0f);
    juce::Colour synthText = juce::Colour::fromFloatRGBA (0.9f, 0.9f, 1.0f, 1.0f);

    // Controls
    juce::ComboBox synthesisModeCombo;
    juce::Slider masterVolumeSlider;
    juce::Slider masterTuneSlider;
    
    // ADSR Envelope
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    
    // Filter
    juce::Slider filterCutoffSlider;
    juce::Slider filterResonanceSlider;
    
    // LFO
    juce::Slider lfoRateSlider;
    juce::Slider lfoAmountSlider;
    juce::ComboBox lfoWaveformCombo;
    juce::ComboBox lfoDestinationCombo;
    
    // Oscillator
    juce::ComboBox oscWaveformCombo;
    
    // Labels
    juce::Label synthesisModeLabel;
    juce::Label masterVolumeLabel;
    juce::Label masterTuneLabel;
    juce::Label titleLabel;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;
    juce::Label filterCutoffLabel;
    juce::Label filterResonanceLabel;
    juce::Label lfoRateLabel;
    juce::Label lfoAmountLabel;
    juce::Label lfoWaveformLabel;
    juce::Label lfoDestinationLabel;
    juce::Label oscWaveformLabel;
    
    // MIDI Keyboard - custom component that fills width
    std::unique_ptr<CustomMidiKeyboard> midiKeyboard;
    juce::MidiKeyboardState keyboardState;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> synthesisModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterVolumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterTuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterResonanceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoRateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoWaveformAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoDestinationAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oscWaveformAttachment;
    
    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& labelText);
    void drawRetroFrame (juce::Graphics& g, juce::Rectangle<int> bounds);
    void applyStyling(); // Apply all styling after construction
    
    // Cached background image for faster painting
    juce::Image backgroundImage;
    bool backgroundImageValid = false;
    bool midiKeyboardConfigured = false;
    bool stylingApplied = false;
    bool isResizingWindow = false; // Prevent recursive resize
    void updateBackgroundImage();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoonrunnerAudioProcessorEditor)
};

