/*
  ==============================================================================

    MoonrunnerStyle.cpp

  ==============================================================================
*/

#include "MoonrunnerStyle.h"

//==============================================================================
float MoonrunnerStyle::getScale (juce::Rectangle<int> bounds)
{
    if (bounds.isEmpty())
        return 1.0f;
    float scaleW = static_cast<float>(bounds.getWidth())  / baseWidth;
    float scaleH = static_cast<float>(bounds.getHeight()) / baseHeight;
    return juce::jmin (scaleW, scaleH);
}

//==============================================================================
void MoonrunnerStyle::drawNeonPanel (juce::Graphics& g, juce::Rectangle<float> r,
                                     juce::Colour borderColour, float borderPx, float cornerRadius,
                                     juce::Colour fillTop, juce::Colour fillBot,
                                     bool innerGlow)
{
    g.saveState();

    // Fill with gradient
    auto fillTopAlpha = fillTop.withAlpha (fillTop.getFloatAlpha() * panelFillOpacity);
    auto fillBotAlpha = fillBot.withAlpha (fillBot.getFloatAlpha() * panelFillOpacity);
    juce::ColourGradient fillGrad (fillTopAlpha, r.getX(), r.getY(),
                                   fillBotAlpha, r.getX(), r.getBottom(), false);
    g.setGradientFill (fillGrad);
    g.fillRoundedRectangle (r, cornerRadius);

    // Inner glow (multiple strokes at decreasing alpha)
    if (innerGlow)
    {
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (borderColour.withAlpha (0.15f / i));
            g.drawRoundedRectangle (r.reduced (borderPx * 0.5f), cornerRadius - borderPx * 0.5f, 0.5f);
        }
    }

    // Border glow (outer strokes)
    for (int i = 3; i >= 1; --i)
    {
        float expand = static_cast<float>(i) * 1.5f;
        g.setColour (borderColour.withAlpha (0.4f / i));
        g.drawRoundedRectangle (r.expanded (expand), cornerRadius + expand, borderPx * 0.5f);
    }

    // Main border
    g.setColour (borderColour);
    g.drawRoundedRectangle (r, cornerRadius, borderPx);

    g.restoreState();
}

//==============================================================================
juce::ColourGradient MoonrunnerStyle::makeLinearGradient (juce::Colour a, juce::Colour b,
                                                          float x0, float y0, float x1, float y1,
                                                          bool vertical)
{
    if (vertical)
        return juce::ColourGradient (a, x0, y0, b, x0, y1, false);
    return juce::ColourGradient (a, x0, y0, b, x1, y0, false);
}

//==============================================================================
void MoonrunnerStyle::drawGradientText (juce::Graphics& g, const juce::String& text,
                                        juce::Font font, juce::Rectangle<int> bounds,
                                        juce::Colour colourA, juce::Colour colourB,
                                        bool horizontalGradient)
{
    g.saveState();

    // Glow/shadow
    g.setColour (colourA.withAlpha (0.5f));
    g.setFont (font);
    g.drawText (text, bounds.translated (0, 1), juce::Justification::centredLeft, true);
    g.drawText (text, bounds.translated (1, 0), juce::Justification::centredLeft, true);

    // Use GlyphArrangement to create path, fill with gradient
    juce::GlyphArrangement ga;
    ga.addFittedText (font, text,
                      static_cast<float>(bounds.getX()), static_cast<float>(bounds.getY()),
                      static_cast<float>(bounds.getWidth()), static_cast<float>(bounds.getHeight()),
                      juce::Justification::centredLeft, 1, 0.0f);

    juce::Path path;
    ga.createPath (path);

    float x1 = static_cast<float>(bounds.getX());
    float y1 = static_cast<float>(bounds.getY());
    float x2 = horizontalGradient ? static_cast<float>(bounds.getRight()) : x1;
    float y2 = horizontalGradient ? y1 : static_cast<float>(bounds.getBottom());

    juce::ColourGradient grad (colourA, x1, y1, colourB, x2, y2, false);
    g.setGradientFill (grad);
    g.fillPath (path);

    g.restoreState();
}

//==============================================================================
void MoonrunnerStyle::drawGridBackground (juce::Graphics& g, juce::Rectangle<int> bounds, float scale)
{
    float spacing = gridSpacing * scale;
    if (spacing < 2.0f)
        spacing = 2.0f;

    g.setColour (neonPink().withAlpha (gridOpacity));

    float x = spacing;
    while (x < bounds.getWidth())
    {
        g.drawLine (x, 0.0f, x, static_cast<float>(bounds.getHeight()), 1.0f);
        x += spacing;
    }

    float y = spacing;
    while (y < bounds.getHeight())
    {
        g.drawLine (0.0f, y, static_cast<float>(bounds.getWidth()), y, 1.0f);
        y += spacing;
    }
}

//==============================================================================
void MoonrunnerStyle::drawGlowOrb (juce::Graphics& g, juce::Point<float> center,
                                   float radius, juce::Colour colour, float blurApprox)
{
    g.saveState();

    float outerRadius = radius * (1.0f + blurApprox * 2.0f);
    juce::ColourGradient radial (colour.withAlpha (0.4f), center.x, center.y,
                                 colour.withAlpha (0.0f), center.x + outerRadius, center.y, true);
    radial.addColour (0.3f, colour.withAlpha (0.15f));
    radial.addColour (0.6f, colour.withAlpha (0.05f));

    g.setGradientFill (radial);
    g.fillEllipse (center.x - outerRadius, center.y - outerRadius,
                   outerRadius * 2.0f, outerRadius * 2.0f);

    // Core
    g.setColour (colour.withAlpha (0.2f));
    g.fillEllipse (center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);

    g.restoreState();
}

//==============================================================================
juce::Font MoonrunnerStyle::getMonoFont (float heightPx, bool bold)
{
    juce::String fontName = juce::Font::getDefaultMonospacedFontName();
    if (fontName.isEmpty())
        fontName = "Courier New";
    return juce::Font (fontName, heightPx, bold ? juce::Font::bold : juce::Font::plain);
}
