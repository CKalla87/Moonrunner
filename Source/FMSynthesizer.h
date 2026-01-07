/*
  ==============================================================================

    FMSynthesizer.h
    FM Synthesis Engine - Inspired by Yamaha DX7

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    FM Synthesis Engine - 6-operator FM synthesis inspired by Yamaha DX7
*/
class FMSynthesizer
{
public:
    FMSynthesizer();
    ~FMSynthesizer() = default;

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();
    
    void noteOn (int midiNoteNumber, float velocity);
    void noteOff (int midiNoteNumber);
    void allNotesOff();
    
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);
    
    // FM Parameters
    void setAlgorithm (int algorithm); // 0-31 (DX7 had 32 algorithms)
    void setOperatorLevel (int operatorIndex, float level); // 0-99
    void setOperatorFrequencyRatio (int operatorIndex, float ratio);
    void setOperatorAttack (int operatorIndex, float attack);
    void setOperatorDecay (int operatorIndex, float decay);
    void setOperatorSustain (int operatorIndex, float sustain);
    void setOperatorRelease (int operatorIndex, float release);
    void setOperatorWaveform (int operatorIndex, int waveform); // 0=sine, 1=triangle, etc.
    void setLFOFrequency (float frequency);
    void setLFOAmount (float amount);
    void setPitchBend (float semitones);
    
private:
    // FM Operator structure
    struct FMOperator
    {
        float phase = 0.0f;
        float phaseIncrement = 0.0f;
        float outputLevel = 0.0f;
        float frequencyRatio = 1.0f;
        float envelopeValue = 0.0f;
        float envelopePhase = 0.0f; // 0=attack, 1=decay, 2=sustain, 3=release
        
        // ADSR
        float attackRate = 0.0f;
        float decayRate = 0.0f;
        float sustainLevel = 0.7f;
        float releaseRate = 0.0f;
        
        int waveform = 0; // 0=sine, 1=triangle, 2=square, 3=sawtooth
        bool isKeyOn = false;
    };
    
    // Voice structure
    struct Voice
    {
        FMOperator operators[6];
        float baseFrequency = 440.0f;
        float velocity = 1.0f;
        bool isActive = false;
        int midiNote = -1;
        float pitchBend = 0.0f;
    };
    
    static constexpr int maxVoices = 16;
    Voice voices[maxVoices];
    
    int currentAlgorithm = 0; // Default algorithm
    float lfoPhase = 0.0f;
    float lfoFrequency = 0.5f;
    float lfoAmount = 0.0f;
    
    double sampleRate = 44100.0;
    
    // Helper functions
    float generateWaveform (const FMOperator& op, float modulation = 0.0f);
    void updateEnvelope (FMOperator& op);
    float processOperator (FMOperator& op, float modulation, float baseFreq);
    int findFreeVoice();
    int findVoiceForNote (int midiNote);
    float midiNoteToFrequency (int midiNote);
    
    // Algorithm routing (simplified - DX7 had complex routing)
    float processAlgorithm (Voice& voice, int algorithm);
};



