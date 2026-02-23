/*
  ==============================================================================

    MoonrunnerStyle.h
    Centralized colors, gradients, shadows, and drawing helpers for MOONRUNNER SYNTH v2.0

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
struct MoonrunnerStyle
{
    // Base design size for scaling
    static constexpr int baseWidth = 1200;
    static constexpr int baseHeight = 865;
    static constexpr float baseAspect = static_cast<float>(baseWidth) / baseHeight;

    // Colors (from React spec)
    static juce::Colour neonPink()      { return juce::Colour (0xffff006e); }
    static juce::Colour neonCyan()      { return juce::Colour (0xff00d9ff); }
    static juce::Colour pinkMid()       { return juce::Colour (0xffff0080); }
    static juce::Colour pinkDark()      { return juce::Colour (0xffd90057); }
    static juce::Colour cyanMid()       { return juce::Colour (0xff00b8d4); }
    static juce::Colour cyanDark()      { return juce::Colour (0xff0096a8); }
    static juce::Colour panelFillTop()  { return juce::Colour (0xff1a1a2e); }
    static juce::Colour panelFillBot()  { return juce::Colour (0xff0f0f1e); }
    static juce::Colour whiteKeyTop()   { return juce::Colour (0xff1a1a2e); }
    static juce::Colour whiteKeyBot()   { return juce::Colour (0xff0a0a14); }
    static juce::Colour blackKeyTop()   { return juce::Colour (0xfff0f0f0); }
    static juce::Colour blackKeyBot()   { return juce::Colour (0xffc0c0c0); }
    static juce::Colour bgDarkTop()     { return juce::Colour (0xff1a0033); }
    static juce::Colour bgDarkMid()     { return juce::Colour (0xff0d001a); }
    static juce::Colour bgDarkBot()     { return juce::Colour (0xff001a33); }

    // Layout constants (base design pixels)
    static constexpr float gridSpacing = 40.0f;
    static constexpr float gridOpacity = 0.30f;
    static constexpr float panelCornerRadius = 27.0f;  // rounded-3xl ~24-30
    static constexpr float panelBorderPx = 4.0f;
    static constexpr float innerPanelBorderPx = 2.0f;   // thinner for section panels
    static constexpr float innerPanelCornerRadius = 24.0f;  // smooth like main outline
    static constexpr float panelFillOpacity = 0.95f;

    //==============================================================================
    /** Compute uniform scale from current bounds vs base size */
    static float getScale (juce::Rectangle<int> bounds);

    /** Draw neon panel with border, fill gradient, optional glow */
    static void drawNeonPanel (juce::Graphics& g, juce::Rectangle<float> r,
                              juce::Colour borderColour, float borderPx, float cornerRadius,
                              juce::Colour fillTop, juce::Colour fillBot,
                              bool innerGlow = false);

    /** Create horizontal linear gradient */
    static juce::ColourGradient makeLinearGradient (juce::Colour a, juce::Colour b,
                                                    float x0, float y0, float x1, float y1,
                                                    bool vertical = false);

    /** Draw text with gradient fill (pink to cyan) and optional glow */
    static void drawGradientText (juce::Graphics& g, const juce::String& text,
                                  juce::Font font, juce::Rectangle<int> bounds,
                                  juce::Colour colourA, juce::Colour colourB,
                                  bool horizontalGradient = true);

    /** Draw neon pink grid background */
    static void drawGridBackground (juce::Graphics& g, juce::Rectangle<int> bounds, float scale = 1.0f);

    /** Draw glow orb via radial gradient */
    static void drawGlowOrb (juce::Graphics& g, juce::Point<float> center,
                             float radius, juce::Colour colour, float blurApprox = 1.0f);

    /** Get monospace font (Courier New or system default) */
    static juce::Font getMonoFont (float heightPx, bool bold = false);
};
