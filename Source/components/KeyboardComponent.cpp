/*
  ==============================================================================

    KeyboardComponent.cpp

  ==============================================================================
*/

#include "KeyboardComponent.h"

//==============================================================================
KeyboardComponent::KeyboardComponent()
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

//==============================================================================
static int whiteKeyIndexToNote (int whiteIdx)
{
    static const int notes[] = { 0, 2, 4, 5, 7, 9, 11 };  // C D E F G A B
    int octave = whiteIdx / 7;
    int key = whiteIdx % 7;
    return 48 + octave * 12 + notes[key];
}

bool KeyboardComponent::isBlackKey (int midiNote) const
{
    int n = midiNote % 12;
    return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;  // C#, D#, F#, G#, A#
}

juce::String KeyboardComponent::getNoteName (int midiNote) const
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int octave = (midiNote / 12) - 1;
    return juce::String (names[midiNote % 12]) + juce::String (octave);
}

// Black keys: (whiteKeyIndex, midiNote) for C3-C5 range
static const std::pair<int, int> blackKeyPositions[] = {
    {0, 49}, {1, 51}, {3, 54}, {4, 56}, {5, 58}, {7, 61}, {8, 63}, {10, 66}, {11, 68}, {12, 70}
};

int KeyboardComponent::getNoteAtPosition (float x, float y)
{
    const float outlineInset = 10.0f;
    auto r = getLocalBounds().toFloat().reduced (outlineInset);
    if (r.isEmpty() || ! r.contains (x, y))
        return -1;
    float cx = x - r.getX();
    float cy = y - r.getY();
    float whiteKeyWidth = r.getWidth() / numWhiteKeys;
    float blackKeyHeight = r.getHeight() * 0.61f;

    // Check black keys first (they're on top)
    if (cy < blackKeyHeight)
    {
        for (const auto& p : blackKeyPositions)
        {
            float left = r.getX() + (p.first + 0.7f) * whiteKeyWidth;
            float bw = whiteKeyWidth * 0.6f;
            if (x >= left && x < left + bw)
                return p.second;
        }
    }

    // White keys
    int whiteIdx = static_cast<int>(cx / whiteKeyWidth);
    if (whiteIdx >= 0 && whiteIdx < numWhiteKeys)
        return whiteKeyIndexToNote (whiteIdx);
    return -1;
}

void KeyboardComponent::paint (juce::Graphics& g)
{
    const float outlineInset = 10.0f;
    const float keyRadius = 6.0f;
    const float keyStroke = 0.5f;
    auto fullBounds = getLocalBounds().toFloat();
    auto r = fullBounds.reduced (outlineInset);
    float whiteKeyWidth = r.getWidth() / numWhiteKeys;
    float whiteKeyHeight = r.getHeight();
    float blackKeyHeight = whiteKeyHeight * 0.61f;

    // White keys (drawn first, behind black)
    for (int w = 0; w < numWhiteKeys; ++w)
    {
        int note = whiteKeyIndexToNote (w);
        if (note < firstNote || note > lastNote) continue;

        float x = r.getX() + w * whiteKeyWidth;
        auto keyRect = juce::Rectangle<float> (x, r.getY(), whiteKeyWidth, whiteKeyHeight);

        bool active = activeNotes.count (note) > 0;

        if (active)
        {
            juce::ColourGradient grad (MoonrunnerStyle::neonPink(), x, r.getY(),
                                       MoonrunnerStyle::pinkDark(), x, r.getBottom(), false);
            grad.addColour (0.5f, MoonrunnerStyle::pinkMid());
            g.setGradientFill (grad);
            g.fillRoundedRectangle (keyRect, keyRadius);
            g.setColour (MoonrunnerStyle::neonPink());
            g.drawRoundedRectangle (keyRect, keyRadius, keyStroke);
            // Glow
            g.setColour (MoonrunnerStyle::neonPink().withAlpha (0.5f));
            g.drawRoundedRectangle (keyRect.expanded (2.0f), keyRadius + 2.0f, keyStroke);
        }
        else
        {
            juce::ColourGradient grad (MoonrunnerStyle::whiteKeyTop(), x, r.getY(),
                                       MoonrunnerStyle::whiteKeyBot(), x, r.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (keyRect, keyRadius);
            g.setColour (MoonrunnerStyle::neonCyan().withAlpha (0.3f));
            g.drawRoundedRectangle (keyRect, keyRadius, keyStroke);
        }

        g.setFont (MoonrunnerStyle::getMonoFont (10.0f * scale, true));
        g.setColour (active ? juce::Colours::white : MoonrunnerStyle::neonCyan().withAlpha (0.6f));
        g.drawText (getNoteName (note), keyRect.withTrimmedBottom (12).toNearestInt(),
                    juce::Justification::centredBottom, true);
    }

    // Black keys (on top)
    for (int note = firstNote; note <= lastNote; ++note)
    {
        if (! isBlackKey (note)) continue;

        int w = -1;
        for (int i = 0; i < numWhiteKeys; ++i)
        {
            int wn = whiteKeyIndexToNote (i);
            int nextNote = (i < numWhiteKeys - 1) ? whiteKeyIndexToNote (i + 1) : 73;
            if (note > wn && note < nextNote) { w = i; break; }
        }
        if (w < 0) continue;

        float left = r.getX() + (w + 0.7f) * whiteKeyWidth;
        float bw = whiteKeyWidth * 0.6f;
        auto keyRect = juce::Rectangle<float> (left, r.getY(), bw, blackKeyHeight);

        bool active = activeNotes.count (note) > 0;

        if (active)
        {
            juce::ColourGradient grad (MoonrunnerStyle::neonCyan(), left, r.getY(),
                                       MoonrunnerStyle::cyanDark(), left, r.getY() + blackKeyHeight, false);
            grad.addColour (0.5f, MoonrunnerStyle::cyanMid());
            g.setGradientFill (grad);
            g.fillRoundedRectangle (keyRect, keyRadius);
            g.setColour (MoonrunnerStyle::neonCyan());
            g.drawRoundedRectangle (keyRect, keyRadius, keyStroke);
            g.setColour (MoonrunnerStyle::neonCyan().withAlpha (0.5f));
            g.drawRoundedRectangle (keyRect.expanded (2.0f), keyRadius + 2.0f, keyStroke);
        }
        else
        {
            juce::ColourGradient grad (MoonrunnerStyle::blackKeyTop(), left, r.getY(),
                                       MoonrunnerStyle::blackKeyBot(), left, r.getY() + blackKeyHeight, false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (keyRect, keyRadius);
            g.setColour (MoonrunnerStyle::neonCyan().withAlpha (0.4f));
            g.drawRoundedRectangle (keyRect, keyRadius, keyStroke);
        }

        g.setFont (MoonrunnerStyle::getMonoFont (9.0f * scale, true));
        g.setColour (active ? juce::Colours::white : juce::Colour (0xff808080));
        g.drawText (getNoteName (note), keyRect.toNearestInt(), juce::Justification::centred, true);
    }

    // Panel outline (light cyan, smooth corners like main)
    g.setColour (MoonrunnerStyle::neonCyan());
    g.drawRoundedRectangle (fullBounds, MoonrunnerStyle::innerPanelCornerRadius * scale,
                            MoonrunnerStyle::innerPanelBorderPx * scale);
}

void KeyboardComponent::resized()
{
}

void KeyboardComponent::mouseDown (const juce::MouseEvent& e)
{
    int note = getNoteAtPosition (static_cast<float>(e.x), static_cast<float>(e.y));
    if (note >= 0 && onNote)
    {
        onNote (note, 1.0f, true);
        addActiveNote (note);
        lastDraggedNote = note;
    }
}

void KeyboardComponent::mouseDrag (const juce::MouseEvent& e)
{
    int note = getNoteAtPosition (static_cast<float>(e.x), static_cast<float>(e.y));
    if (note != lastDraggedNote)
    {
        if (lastDraggedNote >= 0 && onNote)
        {
            onNote (lastDraggedNote, 0.0f, false);
            removeActiveNote (lastDraggedNote);
        }
        if (note >= 0 && onNote)
        {
            onNote (note, 1.0f, true);
            addActiveNote (note);
        }
        lastDraggedNote = note;
    }
}

void KeyboardComponent::mouseUp (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    for (int n : activeNotes)
    {
        if (onNote)
            onNote (n, 0.0f, false);
    }
    clearActiveNotes();
    lastDraggedNote = -1;
}

void KeyboardComponent::mouseExit (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    if (e.mods.isAnyMouseButtonDown())
    {
        for (int n : activeNotes)
        {
            if (onNote)
                onNote (n, 0.0f, false);
        }
        clearActiveNotes();
        lastDraggedNote = -1;
    }
}
