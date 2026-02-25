/*
  ==============================================================================

    PadSynthesizer.h
    Super-wide pad: JUCE Synthesiser + SuperWidePadVoice + Chorus + Reverb

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Pad engine: 12-voice polyphonic synth (each voice 6 unison, PolyBLEP osc,
    drift, ladder filter, micro-delay) + Chorus + Reverb (with HP/LP).
    MIDI is consumed in renderNextBlock; use allNotesOff() on mode switch.
*/
class PadSynthesizer
{
public:
    PadSynthesizer();
    ~PadSynthesizer();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();
    void allNotesOff();

    /** Renders pad and applies chorus + reverb. Consumes MIDI from midiBuffer. */
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          juce::MidiBuffer& midiBuffer,
                          int startSample,
                          int numSamples,
                          juce::AudioProcessorValueTreeState& apvts);

private:
    juce::Synthesiser synth;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb reverb;
    juce::dsp::IIR::Filter<float> revHp, revLp;
    juce::dsp::ProcessSpec fxSpec;
    juce::AudioBuffer<float> fxBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadSynthesizer)
};
