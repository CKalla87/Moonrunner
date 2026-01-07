/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "CustomMidiKeyboard.h"
#include <ctime>
#include <cstdlib>
#include <cstdio>

//==============================================================================
// Forward declare logging function
static void logToFile(const char* message)
{
    static FILE* logFile = nullptr;
    if (logFile == nullptr)
    {
        const char* home = getenv("HOME");
        if (home != nullptr)
        {
            char path[1024];
            snprintf(path, sizeof(path), "%s/moonrunner_debug.log", home);
            logFile = fopen(path, "a");
        }
    }
    if (logFile != nullptr)
    {
        fprintf(logFile, "[%ld] %s\n", time(nullptr), message);
        fflush(logFile);
    }
    fprintf(stderr, "Moonrunner: %s\n", message);
    fflush(stderr);
}

MoonrunnerAudioProcessorEditor::MoonrunnerAudioProcessorEditor (MoonrunnerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    logToFile("Editor constructor start - member init complete");
    DBG("Moonrunner: Editor constructor start");
    
    // CRITICAL: Absolute minimum work in constructor to prevent plugin loading hangs
    logToFile("Setting opaque and size...");
    setOpaque (true);
    // Initial size - will be adjusted in resized() to match keyboard exactly
    // Keyboard: 49 keys (C2-C6) * 20px per key = 980 pixels
    // Increased height to accommodate new controls
    setSize (980, 800); // Exact width: 49 keys * 20px = 980px
    
    logToFile("Editor size set");
    DBG("Moonrunner: Editor size set");
    
    // Only add essential components - defer MIDI keyboard
    logToFile("Adding UI components...");
    addAndMakeVisible (&titleLabel);
    addAndMakeVisible (&synthesisModeCombo);
    addAndMakeVisible (&synthesisModeLabel);
    addAndMakeVisible (&masterVolumeSlider);
    addAndMakeVisible (&masterVolumeLabel);
    addAndMakeVisible (&masterTuneSlider);
    addAndMakeVisible (&masterTuneLabel);
    
    // Add new synth controls
    addAndMakeVisible (&attackSlider);
    addAndMakeVisible (&attackLabel);
    addAndMakeVisible (&decaySlider);
    addAndMakeVisible (&decayLabel);
    addAndMakeVisible (&sustainSlider);
    addAndMakeVisible (&sustainLabel);
    addAndMakeVisible (&releaseSlider);
    addAndMakeVisible (&releaseLabel);
    addAndMakeVisible (&filterCutoffSlider);
    addAndMakeVisible (&filterCutoffLabel);
    addAndMakeVisible (&filterResonanceSlider);
    addAndMakeVisible (&filterResonanceLabel);
    addAndMakeVisible (&lfoRateSlider);
    addAndMakeVisible (&lfoRateLabel);
    addAndMakeVisible (&lfoAmountSlider);
    addAndMakeVisible (&lfoAmountLabel);
    addAndMakeVisible (&lfoWaveformCombo);
    addAndMakeVisible (&lfoWaveformLabel);
    addAndMakeVisible (&lfoDestinationCombo);
    addAndMakeVisible (&lfoDestinationLabel);
    addAndMakeVisible (&oscWaveformCombo);
    addAndMakeVisible (&oscWaveformLabel);
    
    // Basic text setup only
    titleLabel.setText ("MOONRUNNER", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    
    synthesisModeCombo.addItem ("FM Synthesis (DX7)", 1);
    synthesisModeCombo.addItem ("Analog (Prophet/Jupiter/Juno)", 2);
    synthesisModeCombo.addItem ("Sampler (Fairlight)", 3);
    synthesisModeCombo.setSelectedId (1);
    
    synthesisModeLabel.setText ("Synthesis Mode", juce::dontSendNotification);
    synthesisModeLabel.setJustificationType (juce::Justification::centred);
    
    masterVolumeSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    masterVolumeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    masterVolumeLabel.setText ("Master Volume", juce::dontSendNotification);
    masterVolumeLabel.setJustificationType (juce::Justification::centred);
    
    masterTuneSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    masterTuneSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    masterTuneSlider.setTextValueSuffix (" st");
    masterTuneLabel.setText ("Master Tune", juce::dontSendNotification);
    masterTuneLabel.setJustificationType (juce::Justification::centred);
    
    // Setup ADSR sliders
    setupSlider (attackSlider, attackLabel, "Attack");
    setupSlider (decaySlider, decayLabel, "Decay");
    setupSlider (sustainSlider, sustainLabel, "Sustain");
    setupSlider (releaseSlider, releaseLabel, "Release");
    
    // Setup Filter sliders
    setupSlider (filterCutoffSlider, filterCutoffLabel, "Cutoff");
    filterCutoffSlider.setTextValueSuffix (" Hz");
    setupSlider (filterResonanceSlider, filterResonanceLabel, "Resonance");
    
    // Setup LFO sliders
    setupSlider (lfoRateSlider, lfoRateLabel, "LFO Rate");
    lfoRateSlider.setTextValueSuffix (" Hz");
    setupSlider (lfoAmountSlider, lfoAmountLabel, "LFO Amount");
    
    // Setup LFO combos
    lfoWaveformCombo.addItem ("Sine", 1);
    lfoWaveformCombo.addItem ("Triangle", 2);
    lfoWaveformCombo.addItem ("Square", 3);
    lfoWaveformCombo.addItem ("Saw", 4);
    lfoWaveformCombo.setSelectedId (1);
    lfoWaveformLabel.setText ("LFO Wave", juce::dontSendNotification);
    lfoWaveformLabel.setJustificationType (juce::Justification::centred);
    
    lfoDestinationCombo.addItem ("Filter", 1);
    lfoDestinationCombo.addItem ("Pitch", 2);
    lfoDestinationCombo.addItem ("Both", 3);
    lfoDestinationCombo.setSelectedId (1);
    lfoDestinationLabel.setText ("LFO Dest", juce::dontSendNotification);
    lfoDestinationLabel.setJustificationType (juce::Justification::centred);
    
    // Setup Oscillator combo
    oscWaveformCombo.addItem ("Saw", 1);
    oscWaveformCombo.addItem ("Square", 2);
    oscWaveformCombo.addItem ("Pulse", 3);
    oscWaveformCombo.addItem ("Triangle", 4);
    oscWaveformCombo.setSelectedId (1);
    oscWaveformLabel.setText ("Osc Wave", juce::dontSendNotification);
    oscWaveformLabel.setJustificationType (juce::Justification::centred);
    
    // Create parameter attachments FIRST (before MIDI keyboard which may be slow)
    try {
        if (audioProcessor.apvts != nullptr)
        {
            synthesisModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                *audioProcessor.apvts, "SYNTHESIS_MODE", synthesisModeCombo);
            masterVolumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                *audioProcessor.apvts, "MASTER_VOLUME", masterVolumeSlider);
            masterTuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                *audioProcessor.apvts, "MASTER_TUNE", masterTuneSlider);
            
            // ADSR attachments
            attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                *audioProcessor.apvts, "ATTACK", attackSlider);
            decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                *audioProcessor.apvts, "DECAY", decaySlider);
            sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                *audioProcessor.apvts, "SUSTAIN", sustainSlider);
            releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                *audioProcessor.apvts, "RELEASE", releaseSlider);
            
            // Filter attachments
            filterCutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                *audioProcessor.apvts, "FILTER_CUTOFF", filterCutoffSlider);
            filterResonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                *audioProcessor.apvts, "FILTER_RESONANCE", filterResonanceSlider);
            
            // LFO attachments
            lfoRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                *audioProcessor.apvts, "LFO_RATE", lfoRateSlider);
            lfoAmountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                *audioProcessor.apvts, "LFO_AMOUNT", lfoAmountSlider);
            lfoWaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                *audioProcessor.apvts, "LFO_WAVEFORM", lfoWaveformCombo);
            lfoDestinationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                *audioProcessor.apvts, "LFO_DESTINATION", lfoDestinationCombo);
            
            // Oscillator attachment
            oscWaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                *audioProcessor.apvts, "OSC_WAVEFORM", oscWaveformCombo);
        }
    } catch (...) {
        // If parameter attachment fails, plugin can still function
    }
    
    // MIDI keyboard - defer initialization to avoid hangs
    // Will be created and added in resized() when we know the size
    keyboardState.addListener (this);
    
    // All styling deferred to resized() to avoid blocking
    
    logToFile("Editor constructor complete");
    DBG("Moonrunner: Editor constructor complete");
}

MoonrunnerAudioProcessorEditor::~MoonrunnerAudioProcessorEditor()
{
    keyboardState.removeListener (this);
    // midiKeyboard will be automatically destroyed by unique_ptr
}

void MoonrunnerAudioProcessorEditor::handleNoteOn (juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    juce::ignoreUnused (source);
    juce::MidiMessage message = juce::MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity);
    message.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    audioProcessor.processMidiMessage (message);
}

void MoonrunnerAudioProcessorEditor::handleNoteOff (juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    juce::ignoreUnused (source);
    juce::MidiMessage message = juce::MidiMessage::noteOff (midiChannel, midiNoteNumber, velocity);
    message.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    audioProcessor.processMidiMessage (message);
}

void MoonrunnerAudioProcessorEditor::updateKeyboardState (int noteNumber, bool isNoteOn, float velocity)
{
    // Update keyboard state for external MIDI input (thread-safe, called from MessageManager)
    if (isNoteOn)
    {
        keyboardState.noteOn (1, noteNumber, velocity);
    }
    else
    {
        keyboardState.noteOff (1, noteNumber, velocity);
    }
}

void MoonrunnerAudioProcessorEditor::clearKeyboardState()
{
    // Clear all notes (thread-safe, called from MessageManager)
    keyboardState.allNotesOff (1);
}

void MoonrunnerAudioProcessorEditor::setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    // Minimal slider setup - styling applied later
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    slider.setPopupDisplayEnabled (true, false, this);
    addAndMakeVisible (&slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (&label);
}

void MoonrunnerAudioProcessorEditor::applyStyling()
{
    // Apply all styling after construction to speed up plugin loading
    titleLabel.setFont (juce::Font (48.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, synthCyan);
    
    synthesisModeCombo.setColour (juce::ComboBox::backgroundColourId, synthDark);
    synthesisModeCombo.setColour (juce::ComboBox::textColourId, synthText);
    synthesisModeCombo.setColour (juce::ComboBox::outlineColourId, synthCyan);
    synthesisModeCombo.setColour (juce::ComboBox::arrowColourId, synthCyan);
    
    synthesisModeLabel.setFont (juce::Font (16.0f, juce::Font::bold));
    synthesisModeLabel.setColour (juce::Label::textColourId, synthText);
    
    masterVolumeSlider.setColour (juce::Slider::trackColourId, synthCyan);
    masterVolumeSlider.setColour (juce::Slider::thumbColourId, synthMagenta);
    masterVolumeSlider.setColour (juce::Slider::backgroundColourId, synthDark);
    masterVolumeSlider.setColour (juce::Slider::textBoxTextColourId, synthText);
    masterVolumeSlider.setColour (juce::Slider::textBoxBackgroundColourId, synthBlack);
    masterVolumeSlider.setColour (juce::Slider::textBoxOutlineColourId, synthCyan);
    masterVolumeLabel.setFont (juce::Font (14.0f, juce::Font::bold));
    masterVolumeLabel.setColour (juce::Label::textColourId, synthText);
    
    masterTuneSlider.setColour (juce::Slider::trackColourId, synthCyan);
    masterTuneSlider.setColour (juce::Slider::thumbColourId, synthMagenta);
    masterTuneSlider.setColour (juce::Slider::backgroundColourId, synthDark);
    masterTuneSlider.setColour (juce::Slider::textBoxTextColourId, synthText);
    masterTuneSlider.setColour (juce::Slider::textBoxBackgroundColourId, synthBlack);
    masterTuneSlider.setColour (juce::Slider::textBoxOutlineColourId, synthCyan);
    masterTuneLabel.setFont (juce::Font (14.0f, juce::Font::bold));
    masterTuneLabel.setColour (juce::Label::textColourId, synthText);
    
    // Style ADSR sliders
    attackSlider.setColour (juce::Slider::trackColourId, synthCyan);
    attackSlider.setColour (juce::Slider::thumbColourId, synthMagenta);
    attackSlider.setColour (juce::Slider::backgroundColourId, synthDark);
    attackSlider.setColour (juce::Slider::textBoxTextColourId, synthText);
    attackSlider.setColour (juce::Slider::textBoxBackgroundColourId, synthBlack);
    attackSlider.setColour (juce::Slider::textBoxOutlineColourId, synthCyan);
    attackLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    attackLabel.setColour (juce::Label::textColourId, synthText);
    
    decaySlider.setColour (juce::Slider::trackColourId, synthCyan);
    decaySlider.setColour (juce::Slider::thumbColourId, synthMagenta);
    decaySlider.setColour (juce::Slider::backgroundColourId, synthDark);
    decaySlider.setColour (juce::Slider::textBoxTextColourId, synthText);
    decaySlider.setColour (juce::Slider::textBoxBackgroundColourId, synthBlack);
    decaySlider.setColour (juce::Slider::textBoxOutlineColourId, synthCyan);
    decayLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    decayLabel.setColour (juce::Label::textColourId, synthText);
    
    sustainSlider.setColour (juce::Slider::trackColourId, synthCyan);
    sustainSlider.setColour (juce::Slider::thumbColourId, synthMagenta);
    sustainSlider.setColour (juce::Slider::backgroundColourId, synthDark);
    sustainSlider.setColour (juce::Slider::textBoxTextColourId, synthText);
    sustainSlider.setColour (juce::Slider::textBoxBackgroundColourId, synthBlack);
    sustainSlider.setColour (juce::Slider::textBoxOutlineColourId, synthCyan);
    sustainLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    sustainLabel.setColour (juce::Label::textColourId, synthText);
    
    releaseSlider.setColour (juce::Slider::trackColourId, synthCyan);
    releaseSlider.setColour (juce::Slider::thumbColourId, synthMagenta);
    releaseSlider.setColour (juce::Slider::backgroundColourId, synthDark);
    releaseSlider.setColour (juce::Slider::textBoxTextColourId, synthText);
    releaseSlider.setColour (juce::Slider::textBoxBackgroundColourId, synthBlack);
    releaseSlider.setColour (juce::Slider::textBoxOutlineColourId, synthCyan);
    releaseLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    releaseLabel.setColour (juce::Label::textColourId, synthText);
    
    // Style Filter sliders
    filterCutoffSlider.setColour (juce::Slider::trackColourId, synthMagenta);
    filterCutoffSlider.setColour (juce::Slider::thumbColourId, synthCyan);
    filterCutoffSlider.setColour (juce::Slider::backgroundColourId, synthDark);
    filterCutoffSlider.setColour (juce::Slider::textBoxTextColourId, synthText);
    filterCutoffSlider.setColour (juce::Slider::textBoxBackgroundColourId, synthBlack);
    filterCutoffSlider.setColour (juce::Slider::textBoxOutlineColourId, synthMagenta);
    filterCutoffLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    filterCutoffLabel.setColour (juce::Label::textColourId, synthText);
    
    filterResonanceSlider.setColour (juce::Slider::trackColourId, synthMagenta);
    filterResonanceSlider.setColour (juce::Slider::thumbColourId, synthCyan);
    filterResonanceSlider.setColour (juce::Slider::backgroundColourId, synthDark);
    filterResonanceSlider.setColour (juce::Slider::textBoxTextColourId, synthText);
    filterResonanceSlider.setColour (juce::Slider::textBoxBackgroundColourId, synthBlack);
    filterResonanceSlider.setColour (juce::Slider::textBoxOutlineColourId, synthMagenta);
    filterResonanceLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    filterResonanceLabel.setColour (juce::Label::textColourId, synthText);
    
    // Style LFO sliders
    lfoRateSlider.setColour (juce::Slider::trackColourId, synthGreen);
    lfoRateSlider.setColour (juce::Slider::thumbColourId, synthOrange);
    lfoRateSlider.setColour (juce::Slider::backgroundColourId, synthDark);
    lfoRateSlider.setColour (juce::Slider::textBoxTextColourId, synthText);
    lfoRateSlider.setColour (juce::Slider::textBoxBackgroundColourId, synthBlack);
    lfoRateSlider.setColour (juce::Slider::textBoxOutlineColourId, synthGreen);
    lfoRateLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    lfoRateLabel.setColour (juce::Label::textColourId, synthText);
    
    lfoAmountSlider.setColour (juce::Slider::trackColourId, synthGreen);
    lfoAmountSlider.setColour (juce::Slider::thumbColourId, synthOrange);
    lfoAmountSlider.setColour (juce::Slider::backgroundColourId, synthDark);
    lfoAmountSlider.setColour (juce::Slider::textBoxTextColourId, synthText);
    lfoAmountSlider.setColour (juce::Slider::textBoxBackgroundColourId, synthBlack);
    lfoAmountSlider.setColour (juce::Slider::textBoxOutlineColourId, synthGreen);
    lfoAmountLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    lfoAmountLabel.setColour (juce::Label::textColourId, synthText);
    
    // Style LFO combos
    lfoWaveformCombo.setColour (juce::ComboBox::backgroundColourId, synthDark);
    lfoWaveformCombo.setColour (juce::ComboBox::textColourId, synthText);
    lfoWaveformCombo.setColour (juce::ComboBox::outlineColourId, synthGreen);
    lfoWaveformCombo.setColour (juce::ComboBox::arrowColourId, synthGreen);
    lfoWaveformLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    lfoWaveformLabel.setColour (juce::Label::textColourId, synthText);
    
    lfoDestinationCombo.setColour (juce::ComboBox::backgroundColourId, synthDark);
    lfoDestinationCombo.setColour (juce::ComboBox::textColourId, synthText);
    lfoDestinationCombo.setColour (juce::ComboBox::outlineColourId, synthGreen);
    lfoDestinationCombo.setColour (juce::ComboBox::arrowColourId, synthGreen);
    lfoDestinationLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    lfoDestinationLabel.setColour (juce::Label::textColourId, synthText);
    
    // Style Oscillator combo
    oscWaveformCombo.setColour (juce::ComboBox::backgroundColourId, synthDark);
    oscWaveformCombo.setColour (juce::ComboBox::textColourId, synthText);
    oscWaveformCombo.setColour (juce::ComboBox::outlineColourId, synthYellow);
    oscWaveformCombo.setColour (juce::ComboBox::arrowColourId, synthYellow);
    oscWaveformLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    oscWaveformLabel.setColour (juce::Label::textColourId, synthText);
    
    if (midiKeyboard != nullptr)
    {
        midiKeyboard->setWhiteNoteColour (juce::Colours::black);
        midiKeyboard->setBlackNoteColour (synthCyan); // Match background green/cyan
        midiKeyboard->setKeyDownColour (synthMagenta.withAlpha (0.6f));
        midiKeyboard->setMouseOverColour (synthCyan.withAlpha (0.4f));
    }
}

//==============================================================================
void MoonrunnerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Use cached background image if available and valid
    if (backgroundImageValid && backgroundImage.getWidth() == getWidth() && backgroundImage.getHeight() == getHeight())
    {
        g.drawImageAt (backgroundImage, 0, 0);
    }
    else
    {
        // Draw background directly (faster than creating image on first paint)
        juce::ColourGradient gradient (synthBlack, 0, 0,
                                       synthDark, 0, (float) getHeight(),
                                       false);
        gradient.addColour (0.3, synthCyan.withAlpha (0.1f));
        gradient.addColour (0.6, synthMagenta.withAlpha (0.1f));
        g.setGradientFill (gradient);
        g.fillAll();

        // Draw retro frame
        drawRetroFrame (g, getLocalBounds());

        // Draw decorative 80s elements - simplified for faster rendering
        g.setColour (synthCyan.withAlpha (0.3f));
        
        // Draw fewer grid lines for faster rendering
        for (int i = 1; i < 10; i += 2) // Every other line
        {
            float x = (float) getWidth() * (i + 1) / 11.0f;
            g.drawLine (x, 0.0f, x, (float) getHeight(), 0.5f);
        }
        
        for (int i = 1; i < 8; i += 2) // Every other line
        {
            float y = (float) getHeight() * (i + 1) / 9.0f;
            g.drawLine (0.0f, y, (float) getWidth(), y, 0.5f);
        }
    }

    // Draw text overlays
    g.setFont (juce::Font (12.0f, juce::Font::italic));
    g.setColour (synthCyan.withAlpha (0.7f));
    g.drawText ("80s Synthesizer - FM | Analog | Sampler", getWidth() / 2 - 200, 70, 400, 20,
               juce::Justification::centred, false);

    g.setFont (juce::Font (11.0f));
    g.setColour (synthText.withAlpha (0.6f));
    const int keyboardY = getHeight() - 140;
    g.drawText ("Inspired by: DX7, Prophet-5, Jupiter-8, Juno-60/106, Fairlight CMI, Korg M1",
                getWidth() / 2 - 300, keyboardY - 25, 600, 20,
                juce::Justification::centred, false);
    
    // Fill any whitespace after the keyboard
    // JUCE's MidiKeyboardComponent may have internal padding that creates visual whitespace
    if (midiKeyboard != nullptr)
    {
        const int keyboardX = midiKeyboard->getX();
        const int keyboardRightEdge = keyboardX + midiKeyboard->getWidth();
        const int windowWidth = getWidth();
        const int keyboardY = midiKeyboard->getY();
        const int keyboardHeight = midiKeyboard->getHeight();
        
        // Fill gap on the right side if keyboard doesn't reach window edge
        if (keyboardRightEdge < windowWidth)
        {
            const int gapWidth = windowWidth - keyboardRightEdge;
            g.setColour (synthDark);
            g.fillRect (keyboardRightEdge, keyboardY, gapWidth, keyboardHeight);
        }
        
        // Fill gap on the left side if keyboard doesn't start at 0
        if (keyboardX > 0)
        {
            g.setColour (synthDark);
            g.fillRect (0, keyboardY, keyboardX, keyboardHeight);
        }
        
        // Also check if keyboard visually renders smaller than its bounds
        // This can happen if JUCE has internal margins
        // We'll fill the entire keyboard area with background, then let keyboard paint on top
        // This ensures no whitespace shows through
    }
    
    g.setFont (juce::Font (14.0f, juce::Font::bold));
    g.setColour (synthCyan.withAlpha (0.8f));
    g.drawText ("KEYBOARD", getWidth() / 2 - 50, keyboardY - 5, 100, 20,
                juce::Justification::centred, false);
}

void MoonrunnerAudioProcessorEditor::updateBackgroundImage()
{
    if (getWidth() > 0 && getHeight() > 0)
    {
        backgroundImage = juce::Image (juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics g (backgroundImage);
        
        // Draw background
        juce::ColourGradient gradient (synthBlack, 0, 0,
                                       synthDark, 0, (float) getHeight(),
                                       false);
        gradient.addColour (0.3, synthCyan.withAlpha (0.1f));
        gradient.addColour (0.6, synthMagenta.withAlpha (0.1f));
        g.setGradientFill (gradient);
        g.fillAll();
        
        // Draw retro frame
        drawRetroFrame (g, getLocalBounds());
        
        // Draw grid lines
        g.setColour (synthCyan.withAlpha (0.3f));
        for (int i = 0; i < 10; ++i)
        {
            float x = (float) getWidth() * (i + 1) / 11.0f;
            g.drawLine (x, 0.0f, x, (float) getHeight(), 0.5f);
        }
        for (int i = 0; i < 8; ++i)
        {
            float y = (float) getHeight() * (i + 1) / 9.0f;
            g.drawLine (0.0f, y, (float) getWidth(), y, 0.5f);
        }
        
        backgroundImageValid = true;
    }
}

void MoonrunnerAudioProcessorEditor::drawRetroFrame (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Draw 80s-style neon frame
    g.setColour (synthCyan.withAlpha (0.6f));
    
    // Top border with glow effect
    for (int i = 0; i < 3; ++i)
    {
        float alpha = 0.3f + i * 0.2f;
        g.setColour (synthCyan.withAlpha (alpha));
        g.drawLine (0.0f, (float) i, (float) bounds.getWidth(), (float) i, 1.0f);
    }
    
    // Bottom border
    for (int i = 0; i < 3; ++i)
    {
        float alpha = 0.3f + i * 0.2f;
        g.setColour (synthMagenta.withAlpha (alpha));
        g.drawLine (0.0f, (float) (bounds.getHeight() - i), 
                   (float) bounds.getWidth(), (float) (bounds.getHeight() - i), 1.0f);
    }
    
    // Side borders
    g.setColour (synthCyan.withAlpha (0.5f));
    g.drawLine (0.0f, 0.0f, 0.0f, (float) bounds.getHeight(), 2.0f);
    g.drawLine ((float) bounds.getWidth(), 0.0f, (float) bounds.getWidth(), (float) bounds.getHeight(), 2.0f);
    
    // Corner decorations (80s style)
    const int cornerSize = 20;
    g.setColour (synthMagenta);
    
    // Top-left
    g.drawLine (0.0f, 0.0f, (float) cornerSize, 0.0f, 2.0f);
    g.drawLine (0.0f, 0.0f, 0.0f, (float) cornerSize, 2.0f);
    
    // Top-right
    g.drawLine ((float) (bounds.getWidth() - cornerSize), 0.0f, (float) bounds.getWidth(), 0.0f, 2.0f);
    g.drawLine ((float) bounds.getWidth(), 0.0f, (float) bounds.getWidth(), (float) cornerSize, 2.0f);
    
    // Bottom-left
    g.drawLine (0.0f, (float) (bounds.getHeight() - cornerSize), 0.0f, (float) bounds.getHeight(), 2.0f);
    g.drawLine (0.0f, (float) bounds.getHeight(), (float) cornerSize, (float) bounds.getHeight(), 2.0f);
    
    // Bottom-right
    g.drawLine ((float) (bounds.getWidth() - cornerSize), (float) bounds.getHeight(), 
                (float) bounds.getWidth(), (float) bounds.getHeight(), 2.0f);
    g.drawLine ((float) bounds.getWidth(), (float) (bounds.getHeight() - cornerSize), 
                (float) bounds.getWidth(), (float) bounds.getHeight(), 2.0f);
}

void MoonrunnerAudioProcessorEditor::resized()
{
    const int margin = 30;
    const int rotarySize = 80; // Square size for rotary knobs
    const int labelHeight = 25;
    const int spacing = 20; // Tighter spacing for rotary knobs
    
    // Title - centered, moved lower
    titleLabel.setBounds (getWidth() / 2 - 200, 20, 400, 50);

    // Synthesis Mode - centered with more spacing from title (80s text is at y=70, so start below that)
    const int comboWidth = 350;
    const int comboHeight = 30;
    synthesisModeLabel.setBounds (getWidth() / 2 - comboWidth / 2, 100, comboWidth, labelHeight);
    synthesisModeCombo.setBounds (getWidth() / 2 - comboWidth / 2, 125, comboWidth, comboHeight);

    // Top row: Master Volume, Master Tune, Attack, Decay, Sustain, Release, Cutoff, Resonance
    const int masterControlsY = 180;
    const int totalTopRowWidth = 8 * rotarySize + 7 * spacing; // 8 knobs in top row
    const int topRowStartX = (getWidth() - totalTopRowWidth) / 2; // Center the top row
    
    // Store exact X positions for first two knobs to align second row
    const int masterVolumeX = topRowStartX;
    const int masterTuneX = topRowStartX + rotarySize + spacing;
    
    int currentX = topRowStartX;
    
    masterVolumeSlider.setBounds (currentX, masterControlsY, rotarySize, rotarySize);
    masterVolumeLabel.setBounds (currentX, masterControlsY + rotarySize + 5, rotarySize, labelHeight);
    currentX += rotarySize + spacing;
    
    masterTuneSlider.setBounds (currentX, masterControlsY, rotarySize, rotarySize);
    masterTuneLabel.setBounds (currentX, masterControlsY + rotarySize + 5, rotarySize, labelHeight);
    currentX += rotarySize + spacing;
    
    attackSlider.setBounds (currentX, masterControlsY, rotarySize, rotarySize);
    attackLabel.setBounds (currentX, masterControlsY + rotarySize + 5, rotarySize, labelHeight);
    currentX += rotarySize + spacing;
    
    decaySlider.setBounds (currentX, masterControlsY, rotarySize, rotarySize);
    decayLabel.setBounds (currentX, masterControlsY + rotarySize + 5, rotarySize, labelHeight);
    currentX += rotarySize + spacing;
    
    sustainSlider.setBounds (currentX, masterControlsY, rotarySize, rotarySize);
    sustainLabel.setBounds (currentX, masterControlsY + rotarySize + 5, rotarySize, labelHeight);
    currentX += rotarySize + spacing;
    
    releaseSlider.setBounds (currentX, masterControlsY, rotarySize, rotarySize);
    releaseLabel.setBounds (currentX, masterControlsY + rotarySize + 5, rotarySize, labelHeight);
    currentX += rotarySize + spacing;
    
    filterCutoffSlider.setBounds (currentX, masterControlsY, rotarySize, rotarySize);
    filterCutoffLabel.setBounds (currentX, masterControlsY + rotarySize + 5, rotarySize, labelHeight);
    currentX += rotarySize + spacing;
    
    filterResonanceSlider.setBounds (currentX, masterControlsY, rotarySize, rotarySize);
    filterResonanceLabel.setBounds (currentX, masterControlsY + rotarySize + 5, rotarySize, labelHeight);
    
    // Second row: LFO Rate, LFO Amount, LFO Wave, LFO Dest, Osc Wave
    const int secondRowY = masterControlsY + rotarySize + 45; // Space between rows
    const int smallComboWidth = 100;
    const int smallComboHeight = 25;
    const int comboSpacing = 15; // Consistent spacing
    
    // Align second row knobs exactly with top row - use same X positions
    lfoRateSlider.setBounds (masterVolumeX, secondRowY, rotarySize, rotarySize);
    lfoRateLabel.setBounds (masterVolumeX, secondRowY + rotarySize + 5, rotarySize, labelHeight);
    
    lfoAmountSlider.setBounds (masterTuneX, secondRowY, rotarySize, rotarySize);
    lfoAmountLabel.setBounds (masterTuneX, secondRowY + rotarySize + 5, rotarySize, labelHeight);
    
    // Position combos after the knobs
    currentX = masterTuneX + rotarySize + spacing + 15; // Gap after LFO Amount
    
    // Align combo labels and boxes with knob labels (same Y offset)
    const int comboLabelY = secondRowY; // Same Y as knobs
    const int comboBoxY = secondRowY + labelHeight + 2; // Below label, aligned with knob value boxes
    
    lfoWaveformLabel.setBounds (currentX, comboLabelY, smallComboWidth, labelHeight);
    lfoWaveformCombo.setBounds (currentX, comboBoxY, smallComboWidth, smallComboHeight);
    currentX += smallComboWidth + comboSpacing;
    
    lfoDestinationLabel.setBounds (currentX, comboLabelY, smallComboWidth, labelHeight);
    lfoDestinationCombo.setBounds (currentX, comboBoxY, smallComboWidth, smallComboHeight);
    currentX += smallComboWidth + comboSpacing;
    
    oscWaveformLabel.setBounds (currentX, comboLabelY, smallComboWidth, labelHeight);
    oscWaveformCombo.setBounds (currentX, comboBoxY, smallComboWidth, smallComboHeight);
    
    // MIDI Keyboard at the bottom - create and configure here when we know size
    const int keyboardHeight = 120;
    const int keyboardY = getHeight() - keyboardHeight - 20;
    const int keyboardWidth = getWidth(); // Use full width
    
    // Create keyboard if it doesn't exist yet
    if (midiKeyboard == nullptr)
    {
        logToFile("Creating custom MIDI keyboard in resized()...");
        midiKeyboard = std::make_unique<CustomMidiKeyboard>(keyboardState);
        addAndMakeVisible (midiKeyboard.get());
        logToFile("Custom MIDI keyboard created and added");
    }
    
    if (midiKeyboard != nullptr)
    {
        // Configure keyboard - set range: C2 (36) to C6 (84) = 49 keys
        const int lowestNote = 36;  // C2
        const int highestNote = 84;  // C6
        
        midiKeyboard->setAvailableRange (lowestNote, highestNote);
        
        // Custom keyboard automatically fills its bounds - just set the bounds
        const int windowWidth = getWidth();
        midiKeyboard->setBounds (0, keyboardY, windowWidth, keyboardHeight);
        
        // Debug
        char debugMsg[256];
        snprintf(debugMsg, sizeof(debugMsg), 
                 "Custom keyboard: Window=%d, Bounds: x=%d w=%d h=%d", 
                 windowWidth, midiKeyboard->getX(), midiKeyboard->getWidth(), midiKeyboard->getHeight());
        logToFile(debugMsg);
        midiKeyboard->setVisible (true);
        midiKeyboard->toFront (false);
        midiKeyboard->repaint();
        
        if (!midiKeyboardConfigured)
        {
            midiKeyboardConfigured = true;
        }
    }
    
    // Apply styling on first resize (after component is shown)
    if (!stylingApplied)
    {
        applyStyling();
        stylingApplied = true;
    }
    
    // Ensure MIDI keyboard is visible after styling
    if (midiKeyboard != nullptr)
    {
        midiKeyboard->setVisible (true);
        midiKeyboard->toFront (false);
        midiKeyboard->repaint();
    }
    
    // Update background image when resized
    backgroundImageValid = false;
    updateBackgroundImage();
}

