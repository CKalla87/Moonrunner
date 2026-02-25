/*
  ==============================================================================

    SamplerEngine.h
    Industrial Bass Synth - NIN "Head Like a Hole" style

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Industrial bass synth: thick detuned saws, heavy distortion, resonant ladder filter.
    Real-time oscillator-based synthesis - no sample playback.
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

    // Kept for API compatibility (no-op)
    void loadSample (const juce::AudioBuffer<float>&, double) {}
    void clearSample() {}
    void setPlaybackSpeed (float) {}
    void setLoopEnabled (bool) {}
    void setLoopStart (int) {}
    void setLoopEnd (int) {}

    void setAttack (float attackSeconds);
    void setRelease (float releaseSeconds);
    void setFilterCutoff (float cutoffHz);
    void setFilterResonance (float resonance);
    void setPitchBend (float semitones);

private:
    static constexpr int kSawCount = 4;  // Detuned saws
    static constexpr int maxVoices = 8;

    struct Voice
    {
        float phase[kSawCount] = { 0.0f };
        float subPhase = 0.0f;
        float frequency = 110.0f;
        float velocity = 1.0f;
        bool isActive = false;
        int midiNote = -1;
        float pitchBend = 0.0f;
        bool isKeyOn = false;

        float envelopeValue = 0.0f;
        float envelopePhase = 0.0f;  // 0=attack, 1=release
        float attackRate = 0.0f;
        float releaseRate = 0.0f;
    };

    Voice voices[maxVoices];
    float currentAttackTime = 0.003f;   // 3ms - punchy NIN attack
    float currentReleaseTime = 0.4f;    // 400ms release
    float filterCutoffBase = 1800.0f;   // Dark, biting NIN tone
    float filterResonanceBase = 0.75f;  // High resonance for snarl
    double sampleRate = 44100.0;

    juce::dsp::LadderFilter<float> ladderFilter;
    juce::AudioBuffer<float> filterBuffer;

    int findFreeVoice();
    int findVoiceForNote (int midiNote);
    float midiNoteToFrequency (int midiNote);
    void updateEnvelope (Voice& voice);
    float renderVoice (Voice& voice);
};
