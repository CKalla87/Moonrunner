/*
  ==============================================================================

    FMSynthesizer.cpp
    FM Synthesis Engine - Inspired by Yamaha DX7

  ==============================================================================
*/

#include "FMSynthesizer.h"
#include <ctime>
#include <cstdlib>
#include <cstdio>

//==============================================================================
// Forward declare logging
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

FMSynthesizer::FMSynthesizer()
{
    logToFile("FMSynthesizer constructor start");
    DBG("Moonrunner: FMSynthesizer constructor start");
    // Initialize default operator settings
    for (int v = 0; v < maxVoices; ++v)
    {
        for (int op = 0; op < 6; ++op)
        {
            voices[v].operators[op].outputLevel = 0.5f;
            voices[v].operators[op].frequencyRatio = 1.0f;
            voices[v].operators[op].attackRate = 0.01f;
            voices[v].operators[op].decayRate = 0.01f;
            voices[v].operators[op].sustainLevel = 0.7f;
            voices[v].operators[op].releaseRate = 0.01f;
            voices[v].operators[op].waveform = 0; // Sine
        }
    }
    logToFile("FMSynthesizer constructor complete");
    DBG("Moonrunner: FMSynthesizer constructor complete");
}

void FMSynthesizer::prepare (double sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;
    
    // Reset all voices
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].isActive = false;
        for (int op = 0; op < 6; ++op)
        {
            voices[v].operators[op].phase = 0.0f;
            voices[v].operators[op].envelopeValue = 0.0f;
            voices[v].operators[op].envelopePhase = 0.0f;
        }
    }
    
    lfoPhase = 0.0f;
}

void FMSynthesizer::reset()
{
    allNotesOff();
}

float FMSynthesizer::midiNoteToFrequency (int midiNote)
{
    return 440.0f * std::pow (2.0f, (midiNote - 69) / 12.0f);
}

void FMSynthesizer::noteOn (int midiNoteNumber, float velocity)
{
    int voiceIndex = findFreeVoice();
    if (voiceIndex == -1)
        return; // No free voices
    
    Voice& voice = voices[voiceIndex];
    voice.isActive = true;
    voice.midiNote = midiNoteNumber;
    voice.baseFrequency = midiNoteToFrequency (midiNoteNumber);
    voice.velocity = velocity;
    voice.pitchBend = 0.0f;
    
    // Initialize operators
    for (int op = 0; op < 6; ++op)
    {
        voice.operators[op].phase = 0.0f;
        voice.operators[op].envelopeValue = 0.0f;
        voice.operators[op].envelopePhase = 0.0f;
        voice.operators[op].isKeyOn = true;
        
        // Calculate phase increment
        float freq = voice.baseFrequency * voice.operators[op].frequencyRatio;
        voice.operators[op].phaseIncrement = freq / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;
    }
}

void FMSynthesizer::noteOff (int midiNoteNumber)
{
    int voiceIndex = findVoiceForNote (midiNoteNumber);
    if (voiceIndex == -1)
        return;
    
    Voice& voice = voices[voiceIndex];
    for (int op = 0; op < 6; ++op)
    {
        voice.operators[op].isKeyOn = false;
        voice.operators[op].envelopePhase = 3.0f; // Release
    }
}

void FMSynthesizer::allNotesOff()
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive)
        {
            noteOff (voices[v].midiNote);
        }
    }
}

int FMSynthesizer::findFreeVoice()
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (!voices[v].isActive || voices[v].operators[0].envelopeValue < 0.001f)
            return v;
    }
    return -1;
}

int FMSynthesizer::findVoiceForNote (int midiNote)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive && voices[v].midiNote == midiNote)
            return v;
    }
    return -1;
}

float FMSynthesizer::generateWaveform (const FMOperator& op, float modulation)
{
    float phase = op.phase + modulation;
    phase = std::fmod (phase, juce::MathConstants<float>::twoPi);
    
    switch (op.waveform)
    {
        case 0: // Sine
            return std::sin (phase);
        case 1: // Triangle
            return 2.0f * std::abs (phase / juce::MathConstants<float>::pi - 1.0f) - 1.0f;
        case 2: // Square
            return phase < juce::MathConstants<float>::pi ? 1.0f : -1.0f;
        case 3: // Sawtooth
            return 2.0f * (phase / juce::MathConstants<float>::twoPi) - 1.0f;
        default:
            return std::sin (phase);
    }
}

void FMSynthesizer::updateEnvelope (FMOperator& op)
{
    if (op.envelopePhase == 0.0f) // Attack
    {
        op.envelopeValue += op.attackRate;
        if (op.envelopeValue >= 1.0f)
        {
            op.envelopeValue = 1.0f;
            op.envelopePhase = 1.0f; // Move to decay
        }
    }
    else if (op.envelopePhase == 1.0f) // Decay
    {
        op.envelopeValue -= op.decayRate;
        if (op.envelopeValue <= op.sustainLevel)
        {
            op.envelopeValue = op.sustainLevel;
            op.envelopePhase = 2.0f; // Move to sustain
        }
    }
    else if (op.envelopePhase == 2.0f) // Sustain
    {
        if (!op.isKeyOn)
            op.envelopePhase = 3.0f; // Move to release
    }
    else if (op.envelopePhase == 3.0f) // Release
    {
        op.envelopeValue -= op.releaseRate;
        if (op.envelopeValue <= 0.0f)
        {
            op.envelopeValue = 0.0f;
        }
    }
}

float FMSynthesizer::processOperator (FMOperator& op, float modulation, float baseFreq)
{
    updateEnvelope (op);
    
    float output = generateWaveform (op, modulation);
    output *= op.envelopeValue * op.outputLevel;
    
    // Update phase
    float freq = baseFreq * op.frequencyRatio;
    op.phaseIncrement = freq / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;
    op.phase += op.phaseIncrement;
    op.phase = std::fmod (op.phase, juce::MathConstants<float>::twoPi);
    
    return output;
}

float FMSynthesizer::processAlgorithm (Voice& voice, int algorithm)
{
    // Simplified algorithm processing - DX7 had 32 complex algorithms
    // This implements a basic 6-operator stack
    float output = 0.0f;
    
    // Algorithm 0: Simple stack (Op6 -> Op5 -> Op4 -> Op3 -> Op2 -> Op1)
    if (algorithm == 0)
    {
        float mod6 = processOperator (voice.operators[5], 0.0f, voice.baseFrequency);
        float mod5 = processOperator (voice.operators[4], mod6, voice.baseFrequency);
        float mod4 = processOperator (voice.operators[3], mod5, voice.baseFrequency);
        float mod3 = processOperator (voice.operators[2], mod4, voice.baseFrequency);
        float mod2 = processOperator (voice.operators[1], mod3, voice.baseFrequency);
        output = processOperator (voice.operators[0], mod2, voice.baseFrequency);
    }
    // Algorithm 1: Parallel operators feeding carrier
    else if (algorithm == 1)
    {
        float mod1 = processOperator (voice.operators[0], 0.0f, voice.baseFrequency);
        float mod2 = processOperator (voice.operators[1], 0.0f, voice.baseFrequency);
        float mod3 = processOperator (voice.operators[2], 0.0f, voice.baseFrequency);
        float mod4 = processOperator (voice.operators[3], mod1 + mod2, voice.baseFrequency);
        float mod5 = processOperator (voice.operators[4], mod3, voice.baseFrequency);
        output = processOperator (voice.operators[5], mod4 + mod5, voice.baseFrequency);
    }
    // Default: Simple stack
    else
    {
        float mod6 = processOperator (voice.operators[5], 0.0f, voice.baseFrequency);
        float mod5 = processOperator (voice.operators[4], mod6, voice.baseFrequency);
        float mod4 = processOperator (voice.operators[3], mod5, voice.baseFrequency);
        float mod3 = processOperator (voice.operators[2], mod4, voice.baseFrequency);
        float mod2 = processOperator (voice.operators[1], mod3, voice.baseFrequency);
        output = processOperator (voice.operators[0], mod2, voice.baseFrequency);
    }
    
    return output * voice.velocity;
}

void FMSynthesizer::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    // Update LFO
    float lfoIncrement = lfoFrequency / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;
    
    for (int sample = startSample; sample < startSample + numSamples; ++sample)
    {
        float lfoValue = std::sin (lfoPhase) * lfoAmount;
        lfoPhase += lfoIncrement;
        lfoPhase = std::fmod (lfoPhase, juce::MathConstants<float>::twoPi);
        
        float output = 0.0f;
        
        // Process all active voices
        for (int v = 0; v < maxVoices; ++v)
        {
            if (voices[v].isActive)
            {
                // Apply pitch bend
                float freqMultiplier = std::pow (2.0f, voices[v].pitchBend / 12.0f);
                voices[v].baseFrequency = midiNoteToFrequency (voices[v].midiNote) * freqMultiplier;
                
                float voiceOutput = processAlgorithm (voices[v], currentAlgorithm);
                
                // Check if voice should be deactivated
                if (voices[v].operators[0].envelopeValue < 0.001f && !voices[v].operators[0].isKeyOn)
                {
                    voices[v].isActive = false;
                }
                else
                {
                    output += voiceOutput;
                }
            }
        }
        
        // Apply LFO modulation
        output *= (1.0f + lfoValue);
        
        // Write to output buffer
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            outputBuffer.addSample (channel, sample, output * 0.2f); // Scale down
        }
    }
}

void FMSynthesizer::setAlgorithm (int algorithm)
{
    currentAlgorithm = juce::jlimit (0, 31, algorithm);
}

void FMSynthesizer::setOperatorLevel (int operatorIndex, float level)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].operators[operatorIndex].outputLevel = juce::jlimit (0.0f, 1.0f, level / 99.0f);
    }
}

void FMSynthesizer::setOperatorFrequencyRatio (int operatorIndex, float ratio)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].operators[operatorIndex].frequencyRatio = ratio;
    }
}

void FMSynthesizer::setOperatorAttack (int operatorIndex, float attack)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].operators[operatorIndex].attackRate = attack / static_cast<float> (sampleRate);
    }
}

void FMSynthesizer::setOperatorDecay (int operatorIndex, float decay)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].operators[operatorIndex].decayRate = decay / static_cast<float> (sampleRate);
    }
}

void FMSynthesizer::setOperatorSustain (int operatorIndex, float sustain)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].operators[operatorIndex].sustainLevel = juce::jlimit (0.0f, 1.0f, sustain);
    }
}

void FMSynthesizer::setOperatorRelease (int operatorIndex, float release)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].operators[operatorIndex].releaseRate = release / static_cast<float> (sampleRate);
    }
}

void FMSynthesizer::setOperatorWaveform (int operatorIndex, int waveform)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].operators[operatorIndex].waveform = juce::jlimit (0, 3, waveform);
    }
}

void FMSynthesizer::setLFOFrequency (float frequency)
{
    lfoFrequency = juce::jlimit (0.1f, 20.0f, frequency);
}

void FMSynthesizer::setLFOAmount (float amount)
{
    lfoAmount = juce::jlimit (0.0f, 1.0f, amount);
}

void FMSynthesizer::setPitchBend (float semitones)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive)
            voices[v].pitchBend = semitones;
    }
}



