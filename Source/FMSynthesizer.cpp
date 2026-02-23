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
    // Initialize default operator settings with better defaults for good sound
    for (int v = 0; v < maxVoices; ++v)
    {
        // Operator 0 (carrier) - main output
        voices[v].operators[0].outputLevel = 0.8f;
        voices[v].operators[0].frequencyRatio = 1.0f;
        voices[v].operators[0].attackRate = 0.001f; // Fast attack
        voices[v].operators[0].decayRate = 0.0005f; // Medium decay
        voices[v].operators[0].sustainLevel = 0.7f;
        voices[v].operators[0].releaseRate = 0.0003f; // Medium release
        voices[v].operators[0].waveform = 0; // Sine
        
        // Operators 1-2 (modulators) - add character
        for (int op = 1; op < 3; ++op)
        {
            voices[v].operators[op].outputLevel = 0.3f;
            voices[v].operators[op].frequencyRatio = 1.0f + (op * 0.5f); // Slight detuning
            voices[v].operators[op].attackRate = 0.0005f;
            voices[v].operators[op].decayRate = 0.0003f;
            voices[v].operators[op].sustainLevel = 0.5f;
            voices[v].operators[op].releaseRate = 0.0002f;
            voices[v].operators[op].waveform = 0; // Sine
        }
        
        // Operators 3-5 (less prominent)
        for (int op = 3; op < 6; ++op)
        {
            voices[v].operators[op].outputLevel = 0.1f;
            voices[v].operators[op].frequencyRatio = 1.0f;
            voices[v].operators[op].attackRate = 0.001f;
            voices[v].operators[op].decayRate = 0.0005f;
            voices[v].operators[op].sustainLevel = 0.3f;
            voices[v].operators[op].releaseRate = 0.0003f;
            voices[v].operators[op].waveform = 0; // Sine
        }
    }
    logToFile("FMSynthesizer constructor complete");
    DBG("Moonrunner: FMSynthesizer constructor complete");
}

void FMSynthesizer::prepare (double sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;
    
    // Prepare filter for stereo (most common case)
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 2; // Prepare for stereo
    
    filter.prepare (spec);
    updateFilter();
    
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
    filter.reset();
}

void FMSynthesizer::updateFilter()
{
    if (sampleRate > 0.0)
    {
        float qFactor = 0.1f + (filterResonance * 4.9f);
        qFactor = juce::jlimit (0.1f, 5.0f, qFactor);
        *filter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (
            sampleRate, filterCutoffBase, qFactor);
    }
}

float FMSynthesizer::midiNoteToFrequency (int midiNote)
{
    return 440.0f * std::pow (2.0f, (midiNote - 69) / 12.0f);
}

void FMSynthesizer::noteOn (int midiNoteNumber, float velocity)
{
    int voiceIndex = findFreeVoice();
    // findFreeVoice now always returns a valid voice index (steals if needed)
    if (voiceIndex < 0 || voiceIndex >= maxVoices)
        return; // Safety check only
    
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
    // Turn off ALL voices playing this note
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive && voices[v].midiNote == midiNoteNumber)
        {
            Voice& voice = voices[v];
            for (int op = 0; op < 6; ++op)
            {
                voice.operators[op].isKeyOn = false;
                // If in sustain phase, start release from current sustain level
                if (voice.operators[op].envelopePhase == 2.0f) // Sustain
                {
                    voice.operators[op].envelopeValue = voice.operators[op].sustainLevel;
                }
                voice.operators[op].envelopePhase = 3.0f; // Release
            }
        }
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
    // First, try to find a completely inactive voice
    for (int v = 0; v < maxVoices; ++v)
    {
        if (!voices[v].isActive)
            return v;
    }
    
    // If all voices are active, find one that's fully released
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].operators[0].envelopeValue < 0.001f && 
            voices[v].operators[0].envelopePhase == 3.0f && 
            !voices[v].operators[0].isKeyOn)
        {
            // Voice is fully released, can be reused
            voices[v].isActive = false;
            return v;
        }
    }
    
    // If still no free voice, steal one - prefer voices in release phase
    int stealIndex = -1;
    float lowestValue = 1.0f;
    
    // First, look for voices already in release phase
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].operators[0].envelopePhase == 3.0f) // In release phase
        {
            if (voices[v].operators[0].envelopeValue < lowestValue)
            {
                lowestValue = voices[v].operators[0].envelopeValue;
                stealIndex = v;
            }
        }
    }
    
    // If no voice in release, steal the quietest one (regardless of phase)
    // This ensures we always return a voice, even if all are in sustain
    if (stealIndex == -1)
    {
        stealIndex = 0;
        lowestValue = voices[0].operators[0].envelopeValue;
        for (int v = 1; v < maxVoices; ++v)
        {
            if (voices[v].operators[0].envelopeValue < lowestValue)
            {
                lowestValue = voices[v].operators[0].envelopeValue;
                stealIndex = v;
            }
        }
    }
    
    // Steal the voice - ensure operators are properly reset
    if (stealIndex >= 0)
    {
        for (int op = 0; op < 6; ++op)
        {
            voices[stealIndex].operators[op].isKeyOn = false;
            voices[stealIndex].operators[op].envelopePhase = 3.0f; // Release
        }
    }
    
    return stealIndex;
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
    // Normalize phase to [0, 2π] range to prevent artifacts
    float phase = op.phase + modulation;
    phase = std::fmod (phase + juce::MathConstants<float>::twoPi, juce::MathConstants<float>::twoPi);
    
    switch (op.waveform)
    {
        case 0: // Sine - smooth, no aliasing issues
            return std::sin (phase);
        case 1: // Triangle - already smooth
            return 2.0f * std::abs (phase / juce::MathConstants<float>::pi - 1.0f) - 1.0f;
        case 2: // Square - add smoothing to reduce clicks
        {
            // Smooth square wave transition to reduce aliasing
            float softness = 0.01f;
            if (phase < juce::MathConstants<float>::pi - softness)
                return 1.0f;
            else if (phase > juce::MathConstants<float>::pi + softness)
                return -1.0f;
            else
            {
                float t = (phase - (juce::MathConstants<float>::pi - softness)) / (2.0f * softness);
                return 1.0f - 2.0f * t;
            }
        }
        case 3: // Sawtooth - add smoothing at wrap-around
        {
            float saw = 2.0f * (phase / juce::MathConstants<float>::twoPi) - 1.0f;
            // Smooth wrap-around to reduce clicks
            if (phase < 0.02f || phase > juce::MathConstants<float>::twoPi - 0.02f)
            {
                float fade = phase < 0.02f ? phase / 0.02f : (juce::MathConstants<float>::twoPi - phase) / 0.02f;
                saw = saw * fade + (-1.0f) * (1.0f - fade);
            }
            return saw;
        }
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
    
    // Update phase with proper wrapping to prevent discontinuities
    float freq = baseFreq * op.frequencyRatio;
    op.phaseIncrement = freq / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;
    op.phase += op.phaseIncrement;
    
    // Wrap phase smoothly to prevent clicks
    if (op.phase >= juce::MathConstants<float>::twoPi)
        op.phase -= juce::MathConstants<float>::twoPi;
    if (op.phase < 0.0f)
        op.phase += juce::MathConstants<float>::twoPi;
    
    // Prevent excessive output
    output = juce::jlimit (-2.0f, 2.0f, output);
    
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
        
        // Count active voices first to prevent accumulation clipping
        int activeVoiceCount = 0;
        for (int v = 0; v < maxVoices; ++v)
        {
            if (voices[v].isActive)
                activeVoiceCount++;
        }
        
        // Scale factor to prevent clipping when many voices play
        // With 16 max voices, scale down to prevent accumulation distortion
        float voiceScale = activeVoiceCount > 4 ? (4.0f / activeVoiceCount) : 1.0f;
        voiceScale = juce::jlimit (0.25f, 1.0f, voiceScale); // Minimum 0.25x scaling for 16 voices
        
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
                
                // Scale voice output to match Sampler volume
                // Match Sampler: sampleValue * envelopeValue * velocity * 0.7f
                // FM already has velocity, so apply similar scaling
                voiceOutput *= 1.8f; // Base volume
                
                // Scale down if many voices are active to prevent clipping
                voiceOutput *= voiceScale;
                
                // Prevent per-voice clipping
                voiceOutput = juce::jlimit (-1.2f, 1.2f, voiceOutput);
                
                // Check if voice should be deactivated (fully released)
                if (voices[v].operators[0].envelopeValue <= 0.001f && 
                    voices[v].operators[0].envelopePhase == 3.0f && 
                    !voices[v].operators[0].isKeyOn)
                {
                    voices[v].isActive = false;
                    voices[v].operators[0].envelopeValue = 0.0f;
                }
                else
                {
                    output += voiceOutput;
                }
            }
        }
        
        // Apply LFO modulation (very subtle to prevent artifacts)
        output *= (1.0f + lfoValue * 0.05f);
        
        // Normalize output and apply soft saturation instead of hard limiting
        output = juce::jlimit (-1.0f, 1.0f, output);
        
        // Soft saturation for warmth - prevents harsh clipping
        if (std::abs (output) > 0.85f)
        {
            float sign = output > 0.0f ? 1.0f : -1.0f;
            float absOutput = std::abs (output);
            output = sign * (0.85f + 0.15f * std::tanh ((absOutput - 0.85f) * 8.0f));
        }
        
        // Write to output buffer
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            outputBuffer.addSample (channel, sample, output);
        }
    }
    
    // Note: Filter processing removed temporarily to prevent crashes
    // The filter can be re-enabled later with proper per-channel processing
    // For now, the synth will work without the filter
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

void FMSynthesizer::setEnvelopeAttack (float attackSeconds)
{
    float rate = 1.0f / (juce::jmax (0.001f, attackSeconds) * static_cast<float> (sampleRate));
    for (int v = 0; v < maxVoices; ++v)
    {
        for (int op = 0; op < 6; ++op)
        {
            voices[v].operators[op].attackRate = rate;
        }
    }
}

void FMSynthesizer::setEnvelopeDecay (float decaySeconds)
{
    float rate = 1.0f / (juce::jmax (0.001f, decaySeconds) * static_cast<float> (sampleRate));
    for (int v = 0; v < maxVoices; ++v)
    {
        for (int op = 0; op < 6; ++op)
        {
            voices[v].operators[op].decayRate = rate;
        }
    }
}

void FMSynthesizer::setEnvelopeSustain (float sustainLevel)
{
    float level = juce::jlimit (0.0f, 1.0f, sustainLevel);
    for (int v = 0; v < maxVoices; ++v)
    {
        for (int op = 0; op < 6; ++op)
        {
            voices[v].operators[op].sustainLevel = level;
        }
    }
}

void FMSynthesizer::setEnvelopeRelease (float releaseSeconds)
{
    float rate = 1.0f / (juce::jmax (0.01f, releaseSeconds) * static_cast<float> (sampleRate));
    for (int v = 0; v < maxVoices; ++v)
    {
        for (int op = 0; op < 6; ++op)
        {
            voices[v].operators[op].releaseRate = rate;
        }
    }
}

void FMSynthesizer::setFilterCutoff (float cutoffHz)
{
    filterCutoffBase = juce::jlimit (20.0f, 20000.0f, cutoffHz);
    updateFilter();
}

void FMSynthesizer::setFilterResonance (float resonance)
{
    filterResonance = juce::jlimit (0.0f, 1.0f, resonance);
    updateFilter();
}






