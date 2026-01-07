/*
  ==============================================================================

    CustomMidiKeyboard.cpp
    A custom MIDI keyboard component that fills its bounds completely

  ==============================================================================
*/

#include "CustomMidiKeyboard.h"

//==============================================================================
CustomMidiKeyboard::CustomMidiKeyboard (juce::MidiKeyboardState& state)
    : keyboardState (state)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    // Don't add ourselves as a listener - we'll manage pressedNotes directly
    // This avoids callback loops and crashes
}

CustomMidiKeyboard::~CustomMidiKeyboard()
{
    // Clean up any pressed notes
    for (int note : pressedNotes)
    {
        keyboardState.noteOff (1, note, 1.0f);
    }
    pressedNotes.clear();
}

void CustomMidiKeyboard::setAvailableRange (int lowNote, int highNote)
{
    lowestNote = lowNote;
    highestNote = highNote;
    repaint();
}

void CustomMidiKeyboard::handleNoteOn (juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    // This is called by external MIDI input - update visual state
    if (midiNoteNumber >= lowestNote && midiNoteNumber <= highestNote)
    {
        pressedNotes.insert (midiNoteNumber);
        repaint();
    }
}

void CustomMidiKeyboard::handleNoteOff (juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    // This is called by external MIDI input - update visual state
    pressedNotes.erase (midiNoteNumber);
    repaint();
}

// Key width is calculated dynamically to fill width, so this method is not needed
// but kept for API compatibility

void CustomMidiKeyboard::setWhiteNoteColour (juce::Colour colour)
{
    whiteNoteColour = colour;
    repaint();
}

void CustomMidiKeyboard::setBlackNoteColour (juce::Colour colour)
{
    blackNoteColour = colour;
    repaint();
}

void CustomMidiKeyboard::setKeyDownColour (juce::Colour colour)
{
    keyDownColour = colour;
    repaint();
}

void CustomMidiKeyboard::setMouseOverColour (juce::Colour colour)
{
    mouseOverColour = colour;
    repaint();
}

bool CustomMidiKeyboard::isBlackKey (int noteNumber)
{
    int noteInOctave = noteNumber % 12;
    return noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || 
           noteInOctave == 8 || noteInOctave == 10;
}

int CustomMidiKeyboard::getNoteAtPosition (int x, int y)
{
    const int width = getWidth();
    const int height = getHeight();
    const bool isBlackKeyArea = y < height * 0.6f;
    
    // Count white keys
    int whiteKeyCount = 0;
    for (int note = lowestNote; note <= highestNote; ++note)
    {
        if (!isBlackKey (note))
            whiteKeyCount++;
    }
    
    const float whiteKeyWidth = static_cast<float>(width) / static_cast<float>(whiteKeyCount);
    
    // First check black keys (they're on top and clickable)
    if (isBlackKeyArea)
    {
        float currentX = 0.0f;
        for (int note = lowestNote; note <= highestNote; ++note)
        {
            if (!isBlackKey (note))
            {
                // Check for black key after this white key
                if (note + 1 <= highestNote && isBlackKey (note + 1))
                {
                    float blackKeyX = currentX + whiteKeyWidth * 0.65f;
                    float blackKeyW = whiteKeyWidth * 0.6f;
                    
                    if (x >= blackKeyX && x < blackKeyX + blackKeyW)
                    {
                        return note + 1; // Return the black key note
                    }
                }
                
                currentX += whiteKeyWidth;
            }
        }
    }
    
    // Then check white keys
    float currentX = 0.0f;
    for (int note = lowestNote; note <= highestNote; ++note)
    {
        if (!isBlackKey (note))
        {
            if (x >= currentX && x < currentX + whiteKeyWidth)
            {
                return note;
            }
            currentX += whiteKeyWidth;
        }
    }
    
    return -1;
}

void CustomMidiKeyboard::paint (juce::Graphics& g)
{
    const int width = getWidth();
    const int height = getHeight();
    const float whiteKeyHeight = height * 0.95f;
    const float blackKeyHeight = height * 0.6f;
    
    // Calculate how many white keys we have
    int whiteKeyCount = 0;
    for (int note = lowestNote; note <= highestNote; ++note)
    {
        if (!isBlackKey (note))
            whiteKeyCount++;
    }
    
    // Calculate actual key width to fill the entire width
    const float actualKeyWidth = static_cast<float>(width) / static_cast<float>(whiteKeyCount);
    
    // Draw white keys first
    float x = 0.0f;
    for (int note = lowestNote; note <= highestNote; ++note)
    {
        if (!isBlackKey (note))
        {
            juce::Rectangle<float> keyRect (x, 0, actualKeyWidth, whiteKeyHeight);
            
            // Draw white key
            g.setColour (whiteNoteColour);
            g.fillRect (keyRect);
            
            // Draw border
            g.setColour (juce::Colours::grey);
            g.drawRect (keyRect, 1.0f);
            
            // Draw pressed overlay
            if (pressedNotes.find (note) != pressedNotes.end())
            {
                g.setColour (keyDownColour);
                g.fillRect (keyRect);
            }
            
            // Draw mouse over
            if (note == noteUnderMouse)
            {
                g.setColour (mouseOverColour);
                g.fillRect (keyRect);
            }
            
            x += actualKeyWidth;
        }
    }
    
    // Draw black keys on top
    x = 0.0f;
    float lastWhiteKeyX = -actualKeyWidth;
    
    for (int note = lowestNote; note <= highestNote; ++note)
    {
        if (!isBlackKey (note))
        {
            lastWhiteKeyX = x;
            x += actualKeyWidth;
        }
        else
        {
            // Black key is between last white key and next white key
            float blackKeyX = lastWhiteKeyX + actualKeyWidth * 0.65f;
            float blackKeyW = actualKeyWidth * 0.6f;
            
            juce::Rectangle<float> keyRect (blackKeyX, 0, blackKeyW, blackKeyHeight);
            
            // Draw black key
            g.setColour (blackNoteColour);
            g.fillRect (keyRect);
            
            // Draw pressed overlay
            if (pressedNotes.find (note) != pressedNotes.end())
            {
                g.setColour (keyDownColour);
                g.fillRect (keyRect);
            }
            
            // Draw mouse over
            if (note == noteUnderMouse)
            {
                g.setColour (mouseOverColour);
                g.fillRect (keyRect);
            }
        }
    }
}

void CustomMidiKeyboard::resized()
{
    // Component will fill its bounds - keys will scale to fit
    repaint();
}

void CustomMidiKeyboard::mouseDown (const juce::MouseEvent& e)
{
    int note = getNoteAtPosition (e.x, e.y);
    if (note >= 0 && note >= lowestNote && note <= highestNote)
    {
        // Update visual state immediately
        pressedNotes.insert (note);
        noteUnderMouse = note;
        
        // Send MIDI note on
        keyboardState.noteOn (1, note, 1.0f);
        
        repaint();
    }
}

void CustomMidiKeyboard::mouseDrag (const juce::MouseEvent& e)
{
    int note = getNoteAtPosition (e.x, e.y);
    
    // Turn off previous note if different
    if (noteUnderMouse >= 0 && noteUnderMouse != note && noteUnderMouse >= lowestNote && noteUnderMouse <= highestNote)
    {
        if (pressedNotes.find (noteUnderMouse) != pressedNotes.end())
        {
            pressedNotes.erase (noteUnderMouse);
            keyboardState.noteOff (1, noteUnderMouse, 1.0f);
        }
    }
    
    // Turn on new note
    if (note >= 0 && note >= lowestNote && note <= highestNote)
    {
        if (pressedNotes.find (note) == pressedNotes.end())
        {
            pressedNotes.insert (note);
            keyboardState.noteOn (1, note, 1.0f);
        }
        noteUnderMouse = note;
    }
    else
    {
        noteUnderMouse = -1;
    }
    
    repaint();
}

void CustomMidiKeyboard::mouseUp (const juce::MouseEvent& e)
{
    for (int note : pressedNotes)
    {
        keyboardState.noteOff (1, note, 1.0f);
    }
    pressedNotes.clear();
    noteUnderMouse = -1;
    repaint();
}

