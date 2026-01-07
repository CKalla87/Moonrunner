/*
  ==============================================================================

    CustomMidiKeyboard.h
    A custom MIDI keyboard component that fills its bounds completely

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <set>

//==============================================================================
class CustomMidiKeyboard : public juce::Component,
                            public juce::MidiKeyboardState::Listener
{
public:
    CustomMidiKeyboard (juce::MidiKeyboardState& state);
    ~CustomMidiKeyboard() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

    void setAvailableRange (int lowestNote, int highestNote);
    
    // MidiKeyboardState::Listener
    void handleNoteOn (juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    
    // Colours
    void setWhiteNoteColour (juce::Colour colour);
    void setBlackNoteColour (juce::Colour colour);
    void setKeyDownColour (juce::Colour colour);
    void setMouseOverColour (juce::Colour colour);

private:
    juce::MidiKeyboardState& keyboardState;
    
    int lowestNote = 36;  // C2
    int highestNote = 84;  // C6
    float keyWidth = 18.0f;
    
    juce::Colour whiteNoteColour = juce::Colours::white;
    juce::Colour blackNoteColour = juce::Colours::black;
    juce::Colour keyDownColour = juce::Colour (0xff00ffff).withAlpha (0.6f);
    juce::Colour mouseOverColour = juce::Colour (0xff00ffff).withAlpha (0.4f);
    
    int noteUnderMouse = -1;
    std::set<int> pressedNotes;
    
    int getNoteAtPosition (int x, int y);
    bool isBlackKey (int noteNumber);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomMidiKeyboard)
};

