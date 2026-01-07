/*
  ==============================================================================

    SamplerEngine.h
    Sampler Engine - Inspired by Fairlight CMI

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Sampler Engine - Basic sample playback with pitch shifting
*/
class SamplerEngine
{
public:
    SamplerEngine();
    ~SamplerEngine() = default;

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();
    
    void noteOn (int midiNoteNumber, float velocity);
    void noteOff (int midiNoteNumber);
    void allNotesOff();
    
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);
    
    // Sample loading
    void loadSample (const juce::AudioBuffer<float>& sampleData, double originalSampleRate);
    void clearSample();
    
    // Playback parameters
    void setPlaybackSpeed (float speed); // 0.25 to 4.0
    void setLoopEnabled (bool enabled);
    void setLoopStart (int sampleIndex);
    void setLoopEnd (int sampleIndex);
    void setAttack (float attackSeconds);
    void setRelease (float releaseSeconds);
    void setFilterCutoff (float cutoffHz);
    void setFilterResonance (float resonance);
    
    void setPitchBend (float semitones);
    
private:
    struct Voice
    {
        float currentPosition = 0.0f;
        float playbackSpeed = 1.0f;
        float velocity = 1.0f;
        bool isActive = false;
        int midiNote = -1;
        float pitchBend = 0.0f;
        
        // Envelope
        float envelopeValue = 0.0f;
        float envelopePhase = 0.0f; // 0=attack, 1=release
        float attackRate = 0.0f;
        float releaseRate = 0.0f;
        bool isKeyOn = false;
    };
    
    static constexpr int maxVoices = 8;
    Voice voices[maxVoices];
    
    // Sample data
    juce::AudioBuffer<float> sampleBuffer;
    double originalSampleRate = 44100.0;
    bool hasSample = false;
    bool loopEnabled = false;
    int loopStart = 0;
    int loopEnd = 0;
    
    // Envelope parameters (stored to initialize new voices)
    float currentAttackTime = 0.1f;
    float currentReleaseTime = 0.5f;
    
    // Filter (using IIR filter instead of deprecated StateVariableFilter)
    juce::dsp::IIR::Filter<float> filter;
    float filterCutoffBase = 10000.0f;
    float filterResonanceBase = 0.7f;
    
    double sampleRate = 44100.0;
    
    // Helper functions
    int findFreeVoice();
    int findVoiceForNote (int midiNote);
    float midiNoteToFrequency (int midiNote);
    void updateEnvelope (Voice& voice);
    float readSample (const Voice& voice);
    void updateFilter();
};

