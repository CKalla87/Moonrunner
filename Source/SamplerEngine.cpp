/*
  ==============================================================================

    SamplerEngine.cpp
    Sampler Engine - Inspired by Fairlight CMI

  ==============================================================================
*/

#include "SamplerEngine.h"
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

SamplerEngine::SamplerEngine()
{
    logToFile("SamplerEngine constructor start");
    DBG("Moonrunner: SamplerEngine constructor start");
    sampleBuffer.setSize (2, 0); // Stereo, empty initially
    logToFile("SamplerEngine constructor complete");
    DBG("Moonrunner: SamplerEngine constructor complete");
}

void SamplerEngine::prepare (double sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;
    
    // Initialize default envelope times
    currentAttackTime = 0.1f;
    currentReleaseTime = 0.5f;
    
    // Prepare filter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 1;
    
    filter.prepare (spec);
    updateFilter();
    
    // Generate a default sample if none is loaded (simple sawtooth wave)
    // Always generate default sample on prepare to ensure sampler works
    const int defaultSampleLength = static_cast<int> (sampleRate * 2.0); // 2 seconds
    juce::AudioBuffer<float> defaultSample (1, defaultSampleLength);
    
    // Generate a sawtooth wave at A4 (440 Hz)
    const float frequency = 440.0f;
    const float phaseIncrement = frequency / static_cast<float> (sampleRate);
    float phase = 0.0f;
    
    for (int i = 0; i < defaultSampleLength; ++i)
    {
        // Sawtooth: phase goes from -1 to 1
        defaultSample.setSample (0, i, (phase * 2.0f) - 1.0f);
        phase += phaseIncrement;
        if (phase >= 1.0f)
            phase -= 1.0f;
    }
    
    // Always load default sample (will be replaced if user loads custom sample)
    loadSample (defaultSample, sampleRate);
    
    // Reset all voices
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].isActive = false;
        voices[v].currentPosition = 0.0f;
        voices[v].envelopeValue = 0.0f;
        voices[v].envelopePhase = 0.0f;
    }
}

void SamplerEngine::reset()
{
    allNotesOff();
    filter.reset();
}

float SamplerEngine::midiNoteToFrequency (int midiNote)
{
    return 440.0f * std::pow (2.0f, (midiNote - 69) / 12.0f);
}

void SamplerEngine::loadSample (const juce::AudioBuffer<float>& sampleData, double originalSampleRate)
{
    sampleBuffer.makeCopyOf (sampleData);
    this->originalSampleRate = originalSampleRate;
    hasSample = true;
    loopEnd = sampleBuffer.getNumSamples();
}

void SamplerEngine::clearSample()
{
    sampleBuffer.setSize (2, 0);
    hasSample = false;
    allNotesOff();
}

void SamplerEngine::noteOn (int midiNoteNumber, float velocity)
{
    if (!hasSample)
        return;
    
    // First, check if this note is already playing - if so, turn it off first
    // This prevents multiple voices from playing the same note
    int existingVoice = findVoiceForNote (midiNoteNumber);
    if (existingVoice != -1)
    {
        // Turn off the existing voice before starting a new one
        voices[existingVoice].isKeyOn = false;
        voices[existingVoice].envelopePhase = 1.0f; // Release
    }
    
    int voiceIndex = findFreeVoice();
    if (voiceIndex == -1)
        return;
    
    Voice& voice = voices[voiceIndex];
    voice.isActive = true;
    voice.midiNote = midiNoteNumber;
    voice.velocity = velocity;
    voice.pitchBend = 0.0f;
    voice.currentPosition = 0.0f;
    voice.isKeyOn = true;
    voice.envelopeValue = 0.0f;
    voice.envelopePhase = 0.0f;
    
    // Initialize envelope rates based on current attack/release times
    float safeAttack = juce::jmax (0.001f, currentAttackTime);
    voice.attackRate = 1.0f / (safeAttack * static_cast<float> (sampleRate));
    float safeRelease = juce::jmax (0.01f, currentReleaseTime);
    voice.releaseRate = 1.0f / (safeRelease * static_cast<float> (sampleRate));
    
    // Calculate playback speed based on MIDI note
    float targetFreq = midiNoteToFrequency (midiNoteNumber);
    float originalFreq = midiNoteToFrequency (69); // Assuming sample is at A4
    voice.playbackSpeed = (targetFreq / originalFreq) * (static_cast<float> (sampleRate) / static_cast<float> (originalSampleRate));
}

void SamplerEngine::noteOff (int midiNoteNumber)
{
    // Turn off ALL voices playing this note (in case multiple were allocated)
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive && voices[v].midiNote == midiNoteNumber)
        {
            Voice& voice = voices[v];
            voice.isKeyOn = false;
            voice.envelopePhase = 1.0f; // Release
        }
    }
}

void SamplerEngine::allNotesOff()
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive)
        {
            noteOff (voices[v].midiNote);
        }
    }
}

int SamplerEngine::findFreeVoice()
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (!voices[v].isActive || voices[v].envelopeValue < 0.001f)
            return v;
    }
    return -1;
}

int SamplerEngine::findVoiceForNote (int midiNote)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive && voices[v].midiNote == midiNote)
            return v;
    }
    return -1;
}

void SamplerEngine::updateEnvelope (Voice& voice)
{
    if (voice.envelopePhase == 0.0f) // Attack
    {
        voice.envelopeValue += voice.attackRate;
        if (voice.envelopeValue >= 1.0f)
        {
            voice.envelopeValue = 1.0f;
            // Stay in attack phase until key is released
        }
    }
    else if (voice.envelopePhase == 1.0f) // Release
    {
        voice.envelopeValue -= voice.releaseRate;
        if (voice.envelopeValue <= 0.0f)
        {
            voice.envelopeValue = 0.0f;
            // Ensure voice is fully off
        }
    }
}

float SamplerEngine::readSample (const Voice& voice)
{
    if (!hasSample || sampleBuffer.getNumSamples() == 0)
        return 0.0f;
    
    int position = static_cast<int> (voice.currentPosition);
    float fraction = voice.currentPosition - position;
    
    // Handle looping
    if (loopEnabled)
    {
        if (position >= loopEnd)
            position = loopStart + ((position - loopStart) % (loopEnd - loopStart));
        if (position < loopStart)
            position = loopStart;
    }
    else
    {
        if (position >= sampleBuffer.getNumSamples())
            return 0.0f;
    }
    
    // Linear interpolation
    int nextPosition = position + 1;
    if (loopEnabled && nextPosition >= loopEnd)
        nextPosition = loopStart;
    else if (nextPosition >= sampleBuffer.getNumSamples())
        nextPosition = sampleBuffer.getNumSamples() - 1;
    
    float sample1 = sampleBuffer.getSample (0, position);
    float sample2 = sampleBuffer.getSample (0, nextPosition);
    
    return sample1 + (sample2 - sample1) * fraction;
}

void SamplerEngine::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!hasSample)
        return;
    
    for (int sample = startSample; sample < startSample + numSamples; ++sample)
    {
        float output = 0.0f;
        
        // Process all active voices
        for (int v = 0; v < maxVoices; ++v)
        {
            if (voices[v].isActive)
            {
                Voice& voice = voices[v];
                
                // Update envelope
                updateEnvelope (voice);
                
                // Apply pitch bend
                float speedMultiplier = std::pow (2.0f, voice.pitchBend / 12.0f);
                float currentSpeed = voice.playbackSpeed * speedMultiplier;
                
                // Read sample
                float sampleValue = readSample (voice);
                
                // Apply envelope and velocity
                sampleValue *= voice.envelopeValue * voice.velocity;
                
                // Process through filter (using a temporary buffer)
                float filteredSample = sampleValue;
                juce::AudioBuffer<float> tempBuffer (1, 1);
                tempBuffer.setSample (0, 0, filteredSample);
                juce::dsp::AudioBlock<float> block (tempBuffer);
                juce::dsp::ProcessContextReplacing<float> context (block);
                filter.process (context);
                filteredSample = tempBuffer.getSample (0, 0);
                
                output += filteredSample;
                
                // Update position
                voice.currentPosition += currentSpeed;
                
                // Check bounds
                if (loopEnabled)
                {
                    if (voice.currentPosition >= loopEnd)
                        voice.currentPosition = loopStart + std::fmod (voice.currentPosition - loopStart, loopEnd - loopStart);
                }
                else
                {
                    if (voice.currentPosition >= sampleBuffer.getNumSamples())
                    {
                        // Sample ended - start release if not already
                        if (voice.isKeyOn)
                        {
                            voice.isKeyOn = false;
                            voice.envelopePhase = 1.0f; // Release
                        }
                    }
                }
                
                // Check if voice should be deactivated (fully released)
                if (voice.envelopeValue <= 0.001f && !voice.isKeyOn)
                {
                    voice.isActive = false;
                    voice.envelopeValue = 0.0f;
                }
            }
        }
        
        // Write to output buffer
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            outputBuffer.addSample (channel, sample, output * 0.5f); // Scale down
        }
    }
}

void SamplerEngine::setPlaybackSpeed (float speed)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive)
        {
            float baseSpeed = (midiNoteToFrequency (voices[v].midiNote) / midiNoteToFrequency (69)) 
                            * (static_cast<float> (sampleRate) / static_cast<float> (originalSampleRate));
            voices[v].playbackSpeed = baseSpeed * juce::jlimit (0.25f, 4.0f, speed);
        }
    }
}

void SamplerEngine::setLoopEnabled (bool enabled)
{
    loopEnabled = enabled;
}

void SamplerEngine::setLoopStart (int sampleIndex)
{
    loopStart = juce::jlimit (0, sampleBuffer.getNumSamples() - 1, sampleIndex);
}

void SamplerEngine::setLoopEnd (int sampleIndex)
{
    loopEnd = juce::jlimit (loopStart + 1, sampleBuffer.getNumSamples(), sampleIndex);
}

void SamplerEngine::setAttack (float attackSeconds)
{
    // Store the attack time for new voices
    currentAttackTime = attackSeconds;
    
    // Ensure minimum attack time
    float safeAttack = juce::jmax (0.001f, attackSeconds);
    float rate = 1.0f / (safeAttack * static_cast<float> (sampleRate));
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].attackRate = rate;
    }
}

void SamplerEngine::setRelease (float releaseSeconds)
{
    // Store the release time for new voices
    currentReleaseTime = releaseSeconds;
    
    // Ensure minimum release time to prevent stuck notes
    float safeRelease = juce::jmax (0.01f, releaseSeconds);
    float rate = 1.0f / (safeRelease * static_cast<float> (sampleRate));
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].releaseRate = rate;
    }
}

void SamplerEngine::setFilterCutoff (float cutoffHz)
{
    filterCutoffBase = juce::jlimit (20.0f, 20000.0f, cutoffHz);
    updateFilter();
}

void SamplerEngine::setFilterResonance (float resonance)
{
    // Map 0.0-1.0 resonance to Q factor 0.1 to 5.0 (prevent instability)
    filterResonanceBase = juce::jlimit (0.0f, 1.0f, resonance);
    updateFilter();
}

void SamplerEngine::updateFilter()
{
    // Map resonance (0.0-1.0) to Q factor (0.1 to 5.0)
    // Higher resonance = higher Q, but cap at 5.0 to prevent instability
    float qFactor = 0.1f + (filterResonanceBase * 4.9f);
    *filter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (
        sampleRate, filterCutoffBase, qFactor);
}

void SamplerEngine::setPitchBend (float semitones)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive)
            voices[v].pitchBend = semitones;
    }
}

