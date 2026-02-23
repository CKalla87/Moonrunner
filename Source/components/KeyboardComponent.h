/*
  ==============================================================================

    KeyboardComponent.h
    Custom keyboard C3-C5, inverted visual (dark white keys, light black keys)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../ui/MoonrunnerStyle.h"
#include <set>
#include <functional>
#include <utility>

//==============================================================================
class KeyboardComponent : public juce::Component
{
public:
    KeyboardComponent();
    ~KeyboardComponent() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setScale (float s) { scale = s; }
    void setActiveNotes (const std::set<int>& notes) { activeNotes = notes; repaint(); }
    void addActiveNote (int note) { activeNotes.insert (note); repaint(); }
    void removeActiveNote (int note) { activeNotes.erase (note); repaint(); }
    void clearActiveNotes() { activeNotes.clear(); repaint(); }

    /** Callback: (midiNoteNumber, velocity 0-1, isNoteOn) */
    std::function<void(int, float, bool)> onNote;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

private:
    int getNoteAtPosition (float x, float y);
    bool isBlackKey (int midiNote) const;
    juce::String getNoteName (int midiNote) const;

    std::set<int> activeNotes;
    int lastDraggedNote = -1;
    float scale = 1.0f;

    static constexpr int firstNote = 48;   // C3
    static constexpr int lastNote = 72;    // C5
    static constexpr int numWhiteKeys = 15;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyboardComponent)
};
