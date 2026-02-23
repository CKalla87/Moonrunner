/*
  ==============================================================================

    AnalogSynthesizer.h
    Analog Synthesis Engine - Inspired by Prophet-5, Jupiter-8, Juno-60/106

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Analog Synthesis Engine - Classic subtractive synthesis
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
    
    // Oscillator parameters
    void setOscillatorWaveform (int oscIndex, int waveform); // 0=saw, 1=square, 2=pulse, 3=triangle
    void setOscillatorOctave (int oscIndex, int octave); // -2 to +2
    void setOscillatorTune (int oscIndex, float semitones); // -12 to +12
    void setOscillatorLevel (int oscIndex, float level); // 0.0 to 1.0
    void setPulseWidth (int oscIndex, float pulseWidth); // 0.0 to 1.0 (for pulse wave)
    
    // Filter parameters (24dB/octave lowpass like Prophet-5)
    void setFilterCutoff (float cutoffHz);
    void setFilterResonance (float resonance); // 0.0 to 1.0
    void setFilterEnvelopeAmount (float amount); // -1.0 to 1.0
    void setFilterKeyboardTracking (float tracking); // 0.0 to 1.0
    
    // Envelope parameters (ADSR)
    void setEnvelopeAttack (float attackSeconds);
    void setEnvelopeDecay (float decaySeconds);
    void setEnvelopeSustain (float sustainLevel); // 0.0 to 1.0
    void setEnvelopeRelease (float releaseSeconds);
    
    // Filter envelope parameters
    void setFilterEnvelopeAttack (float attackSeconds);
    void setFilterEnvelopeDecay (float decaySeconds);
    void setFilterEnvelopeSustain (float sustainLevel);
    void setFilterEnvelopeRelease (float releaseSeconds);
    
    // LFO parameters
    void setLFORate (float rateHz);
    void setLFOAmount (float amount); // 0.0 to 1.0
    void setLFOWaveform (int waveform); // 0=sine, 1=triangle, 2=square, 3=saw
    void setLFODestination (int destination); // 0=filter, 1=pitch, 2=both
    
    // Chorus (Juno-60/106 style)
    void setChorusEnabled (bool enabled);
    void setChorusRate (float rateHz);
    void setChorusDepth (float depth);
    
    // Sub oscillator (Juno style)
    void setSubOscillatorEnabled (bool enabled);
    void setSubOscillatorOctave (int octave); // -1 or -2
    
    void setPitchBend (float semitones);
    
private:
    struct Oscillator
    {
        float phase = 0.0f;
        float phaseIncrement = 0.0f;
        int waveform = 0; // 0=saw, 1=square, 2=pulse, 3=triangle
        float level = 0.5f;
        int octave = 0;
        float tune = 0.0f; // semitones
        float pulseWidth = 0.5f;
    };
    
    struct Envelope
    {
        float value = 0.0f;
        float phase = 0.0f; // 0=attack, 1=decay, 2=sustain, 3=release
        float attackRate = 0.0f;
        float decayRate = 0.0f;
        float sustainLevel = 0.7f;
        float releaseRate = 0.0f;
        bool isKeyOn = false;
    };
    
    struct Voice
    {
        Oscillator oscillators[2]; // Two oscillators like Prophet-5
        Envelope ampEnvelope;
        Envelope filterEnvelope;
        float baseFrequency = 440.0f;
        float velocity = 1.0f;
        bool isActive = false;
        int midiNote = -1;
        float pitchBend = 0.0f;
        float filterCutoff = 1000.0f;
        float lastFiltered = 0.0f; // Per-voice filter state
        float lastEnvValue = 0.0f; // Per-voice envelope smoothing
    };
    
    static constexpr int maxVoices = 8; // Polyphonic like Prophet-5
    Voice voices[maxVoices];
    
    // Filter (24dB/octave lowpass - using IIR filter)
    juce::dsp::IIR::Filter<float> filter;
    float filterCutoffBase = 1000.0f;
    float filterResonance = 0.7f;
    float filterEnvAmount = 0.5f;
    float filterKeyTracking = 0.5f;
    
    // LFO
    float lfoPhase = 0.0f;
    float lfoRate = 1.0f;
    float lfoAmount = 0.0f;
    int lfoWaveform = 0;
    int lfoDestination = 0; // 0=filter, 1=pitch, 2=both
    
    // Chorus (Juno style)
    bool chorusEnabled = false;
    float chorusPhase[3] = {0.0f, 0.0f, 0.0f};
    float chorusRate = 0.5f;
    float chorusDepth = 0.3f;
    juce::dsp::DelayLine<float> chorusDelay[3];
    
    // Sub oscillator
    bool subOscEnabled = false;
    int subOscOctave = -1;
    float subOscPhase = 0.0f;
    
    double sampleRate = 44100.0;
    
    // Helper functions
    float generateWaveform (const Oscillator& osc);
    void updateEnvelope (Envelope& env);
    int findFreeVoice();
    int findVoiceForNote (int midiNote);
    float midiNoteToFrequency (int midiNote);
    float generateLFO();
    float processChorus (float input);
    void updateFilter();
};

