/*
  ==============================================================================

    AnalogSynthesizer.h
    Van Halen "Jump" - Oberheim OB-Xa brass synth (1:1 recreation)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Van Halen Jump synth - Oberheim OB-Xa brass patch
    Dual detuned saws, 24dB ladder filter, filter+amp envelopes, chorus, Marshall-style saturation
*/
class AnalogSynthesizer
{
public:
    AnalogSynthesizer();
    ~AnalogSynthesizer() = default;

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    void noteOn (int midiNoteNumber, float velocity);
    void noteOff (int midiNoteNumber);
    void allNotesOff();

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);

    // Parameter setters (kept for PluginProcessor compatibility - Jump sound is fixed)
    void setOscillatorWaveform (int oscIndex, int waveform);
    void setOscillatorOctave (int oscIndex, int octave);
    void setOscillatorTune (int oscIndex, float semitones);
    void setOscillatorLevel (int oscIndex, float level);
    void setPulseWidth (int oscIndex, float pulseWidth);
    void setFilterCutoff (float cutoffHz);
    void setFilterResonance (float resonance);
    void setFilterEnvelopeAmount (float amount);
    void setFilterKeyboardTracking (float tracking);
    void setEnvelopeAttack (float attackSeconds);
    void setEnvelopeDecay (float decaySeconds);
    void setEnvelopeSustain (float sustainLevel);
    void setEnvelopeRelease (float releaseSeconds);
    void setFilterEnvelopeAttack (float attackSeconds);
    void setFilterEnvelopeDecay (float decaySeconds);
    void setFilterEnvelopeSustain (float sustainLevel);
    void setFilterEnvelopeRelease (float releaseSeconds);
    void setLFORate (float rateHz);
    void setLFOAmount (float amount);
    void setLFOWaveform (int waveform);
    void setLFODestination (int destination);
    void setChorusEnabled (bool enabled);
    void setChorusRate (float rateHz);
    void setChorusDepth (float depth);
    void setSubOscillatorEnabled (bool enabled);
    void setSubOscillatorOctave (int octave);
    void setPitchBend (float semitones);

private:
    struct JumpEnvelope
    {
        float value = 0.0f;
        float phase = 0.0f;
        float attackRate = 0.0f;
        float decayRate = 0.0f;
        float sustainLevel = 0.0f;
        float releaseRate = 0.0f;
        bool isKeyOn = false;
    };

    struct JumpVoice
    {
        float phase[2] = {0.0f, 0.0f};
        float phaseIncrement[2] = {0.0f, 0.0f};
        JumpEnvelope ampEnv;
        JumpEnvelope filterEnv;
        float baseFreq = 440.0f;
        float velocity = 1.0f;
        bool isActive = false;
        int midiNote = -1;
        float pitchBend = 0.0f;
    };

    static constexpr int maxVoices = 8;
    JumpVoice voices[maxVoices];

    juce::dsp::LadderFilter<float> voiceFilters[maxVoices];
    juce::AudioBuffer<float> filterBuffer;

    juce::dsp::DelayLine<float> chorusDelay[2];
    float chorusPhase[2] = {0.0f, 0.0f};

    double sampleRate = 44100.0;

    // Jump constants (Oberheim OB-Xa brass - hardcoded)
    static constexpr float osc1Level = 1.0f;
    static constexpr float osc2Level = 0.92f;
    static constexpr float osc2DetuneCents = 12.0f;
    static constexpr float filterCutoffBase = 3200.0f;
    static constexpr float filterResonance = 0.28f;
    static constexpr float filterEnvAmount = 0.75f;
    static constexpr float filterKeyTracking = 0.6f;
    static constexpr float ampAttackSec = 0.003f;
    static constexpr float ampDecaySec = 0.15f;
    static constexpr float ampSustain = 0.82f;
    static constexpr float ampReleaseSec = 0.28f;
    static constexpr float filtAttackSec = 0.006f;
    static constexpr float filtDecaySec = 0.4f;
    static constexpr float filtSustain = 0.42f;
    static constexpr float filtReleaseSec = 0.22f;
    static constexpr float chorusRateHz = 0.6f;
    static constexpr float chorusDepth = 0.35f;

    void updateEnvelope (JumpEnvelope& env);
    float saw (float phase);
    int findFreeVoice();
    int findVoiceForNote (int midiNote);
    float midiNoteToFrequency (int midiNote);
};
