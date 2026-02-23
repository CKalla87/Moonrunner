/*
  ==============================================================================

    AnalogSynthesizer.cpp
    Analog Synthesis Engine - Inspired by Prophet-5, Jupiter-8, Juno-60/106

  ==============================================================================
*/

#include "AnalogSynthesizer.h"
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

AnalogSynthesizer::AnalogSynthesizer()
{
    logToFile("AnalogSynthesizer constructor start");
    DBG("Moonrunner: AnalogSynthesizer constructor start");
    // Initialize voices
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].oscillators[0].waveform = 0; // Saw
        voices[v].oscillators[0].level = 1.0f; // Maximum level for loud output
        voices[v].oscillators[0].octave = 0;
        voices[v].oscillators[1].waveform = 1; // Square
        voices[v].oscillators[1].level = 1.0f; // Maximum level for loud output
        voices[v].oscillators[1].octave = 0;
        
        voices[v].ampEnvelope.sustainLevel = 0.7f;
        voices[v].filterEnvelope.sustainLevel = 0.7f;
    }
    
    // Chorus delays will be initialized in prepare()
    logToFile("AnalogSynthesizer constructor complete");
    DBG("Moonrunner: AnalogSynthesizer constructor complete");
}

void AnalogSynthesizer::prepare (double sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;
    
    // Prepare filter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 1;
    
    filter.prepare (spec);
    updateFilter();
    
    // Prepare chorus delays - use reasonable maximum to avoid large allocations
    const int maxDelaySamples = static_cast<int> (sampleRate * 0.1); // 100ms max
    for (int i = 0; i < 3; ++i)
    {
        chorusDelay[i].prepare (spec);
        chorusDelay[i].setMaximumDelayInSamples (maxDelaySamples);
        // Set initial delay based on actual sample rate
        float baseDelay = static_cast<float> (sampleRate * 0.05f); // 50ms base
        chorusDelay[i].setDelay (baseDelay + (i * baseDelay * 0.1f));
    }
    
    // Reset all voices
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].isActive = false;
        voices[v].oscillators[0].phase = 0.0f;
        voices[v].oscillators[1].phase = 0.0f;
        voices[v].ampEnvelope.value = 0.0f;
        voices[v].ampEnvelope.phase = 0.0f;
        voices[v].filterEnvelope.value = 0.0f;
        voices[v].filterEnvelope.phase = 0.0f;
        voices[v].lastFiltered = 0.0f;
        voices[v].lastEnvValue = 0.0f;
    }
    
    lfoPhase = 0.0f;
    for (int i = 0; i < 3; ++i)
        chorusPhase[i] = 0.0f;
    subOscPhase = 0.0f;
}

void AnalogSynthesizer::reset()
{
    allNotesOff();
    filter.reset();
    for (int i = 0; i < 3; ++i)
        chorusDelay[i].reset();
}

float AnalogSynthesizer::midiNoteToFrequency (int midiNote)
{
    return 440.0f * std::pow (2.0f, (midiNote - 69) / 12.0f);
}

void AnalogSynthesizer::noteOn (int midiNoteNumber, float velocity)
{
    // First, check if this note is already playing - if so, turn it off first
    // This prevents multiple voices from playing the same note
    int existingVoice = findVoiceForNote (midiNoteNumber);
    if (existingVoice != -1)
    {
        // Turn off the existing voice before starting a new one
        voices[existingVoice].ampEnvelope.isKeyOn = false;
        voices[existingVoice].ampEnvelope.phase = 3.0f; // Release
        voices[existingVoice].filterEnvelope.isKeyOn = false;
        voices[existingVoice].filterEnvelope.phase = 3.0f; // Release
    }
    
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
    
    // Initialize oscillators
    for (int osc = 0; osc < 2; ++osc)
    {
        float freq = voice.baseFrequency * std::pow (2.0f, voice.oscillators[osc].octave + voice.oscillators[osc].tune / 12.0f);
        voice.oscillators[osc].phase = 0.0f;
        voice.oscillators[osc].phaseIncrement = freq / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;
    }
    
    // Initialize envelopes
    voice.ampEnvelope.value = 0.0f;
    voice.ampEnvelope.phase = 0.0f;
    voice.ampEnvelope.isKeyOn = true;
    
    voice.filterEnvelope.value = 0.0f;
    voice.filterEnvelope.phase = 0.0f;
    voice.filterEnvelope.isKeyOn = true;
    
    // Calculate filter cutoff with key tracking
    float keyTracking = (midiNoteNumber - 60) / 12.0f * filterKeyTracking;
    voice.filterCutoff = filterCutoffBase * std::pow (2.0f, keyTracking);
}

void AnalogSynthesizer::noteOff (int midiNoteNumber)
{
    // Turn off ALL voices playing this note (in case multiple were allocated)
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive && voices[v].midiNote == midiNoteNumber)
        {
            Voice& voice = voices[v];
            voice.ampEnvelope.isKeyOn = false;
            // Start release from current envelope value (could be in sustain phase)
            if (voice.ampEnvelope.phase == 2.0f) // If in sustain, start release from sustain level
                voice.ampEnvelope.value = voice.ampEnvelope.sustainLevel;
            voice.ampEnvelope.phase = 3.0f; // Release
            
            voice.filterEnvelope.isKeyOn = false;
            if (voice.filterEnvelope.phase == 2.0f)
                voice.filterEnvelope.value = voice.filterEnvelope.sustainLevel;
            voice.filterEnvelope.phase = 3.0f; // Release
        }
    }
}

void AnalogSynthesizer::allNotesOff()
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive)
        {
            noteOff (voices[v].midiNote);
        }
    }
}

int AnalogSynthesizer::findFreeVoice()
{
    // First, try to find a completely inactive voice
    for (int v = 0; v < maxVoices; ++v)
    {
        if (!voices[v].isActive)
            return v;
    }
    
    // If all voices are active, find one that's fully released (in release phase and envelope is zero)
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].ampEnvelope.value < 0.001f && voices[v].ampEnvelope.phase == 3.0f)
        {
            // Voice is fully released, can be reused
            voices[v].isActive = false;
            return v;
        }
    }
    
    // If still no free voice, steal one - prefer voices in release phase (quieter)
    // First, look for voices already in release phase
    int stealIndex = -1;
    float lowestValue = 1.0f;
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].ampEnvelope.phase == 3.0f) // In release phase
        {
            if (voices[v].ampEnvelope.value < lowestValue)
            {
                lowestValue = voices[v].ampEnvelope.value;
                stealIndex = v;
            }
        }
    }
    
    // If no voice in release, steal the quietest one (regardless of phase)
    // This ensures we always return a voice, even if all are in sustain
    if (stealIndex == -1)
    {
        stealIndex = 0;
        lowestValue = voices[0].ampEnvelope.value;
        for (int v = 1; v < maxVoices; ++v)
        {
            if (voices[v].ampEnvelope.value < lowestValue)
            {
                lowestValue = voices[v].ampEnvelope.value;
                stealIndex = v;
            }
        }
    }
    
    // Steal the voice - ensure it's properly reset
    if (stealIndex >= 0)
    {
        voices[stealIndex].ampEnvelope.isKeyOn = false;
        voices[stealIndex].ampEnvelope.phase = 3.0f; // Release
        voices[stealIndex].filterEnvelope.isKeyOn = false;
        voices[stealIndex].filterEnvelope.phase = 3.0f; // Release
    }
    
    return stealIndex;
}

int AnalogSynthesizer::findVoiceForNote (int midiNote)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive && voices[v].midiNote == midiNote)
            return v;
    }
    return -1;
}

float AnalogSynthesizer::generateWaveform (const Oscillator& osc)
{
    // Normalize phase to [0, 2π] range - use fmod for smooth wrapping
    float phase = std::fmod (osc.phase + juce::MathConstants<float>::twoPi, juce::MathConstants<float>::twoPi);
    
    switch (osc.waveform)
    {
        case 0: // Sawtooth - improved smoothing at wrap-around to prevent clicks
        {
            // Improved sawtooth with better smoothing at wrap-around
            float saw = 2.0f * (phase / juce::MathConstants<float>::twoPi) - 1.0f;
            // Apply gentle smoothing near wrap-around to reduce clicks without reducing volume
            float wrapZone = 0.005f; // Smaller zone for smoother transition
            if (phase < wrapZone)
            {
                // Smooth fade-in from -1.0
                float fade = phase / wrapZone;
                saw = -1.0f + (saw + 1.0f) * fade;
            }
            else if (phase > juce::MathConstants<float>::twoPi - wrapZone)
            {
                // Smooth fade-out to -1.0
                float fade = (juce::MathConstants<float>::twoPi - phase) / wrapZone;
                float nextSaw = -1.0f; // Value after wrap
                saw = saw * fade + nextSaw * (1.0f - fade);
            }
            return saw;
        }
        case 1: // Square - use smoothed transition
        {
            // Soft square wave with slight smoothing to reduce aliasing
            float softness = 0.02f; // Small transition zone
            if (phase < juce::MathConstants<float>::pi - softness)
                return 1.0f;
            else if (phase > juce::MathConstants<float>::pi + softness)
                return -1.0f;
            else
            {
                // Smooth transition
                float t = (phase - (juce::MathConstants<float>::pi - softness)) / (2.0f * softness);
                return 1.0f - 2.0f * t;
            }
        }
        case 2: // Pulse
        {
            float threshold = osc.pulseWidth * juce::MathConstants<float>::twoPi;
            // Smooth pulse wave transition
            float softness = 0.02f;
            if (phase < threshold - softness)
                return 1.0f;
            else if (phase > threshold + softness)
                return -1.0f;
            else
            {
                float t = (phase - (threshold - softness)) / (2.0f * softness);
                return 1.0f - 2.0f * t;
            }
        }
        case 3: // Triangle - already smooth
            return 2.0f * std::abs (phase / juce::MathConstants<float>::pi - 1.0f) - 1.0f;
        case 4: // Sine
            return std::sin (phase);
        default:
            return 2.0f * (phase / juce::MathConstants<float>::twoPi) - 1.0f;
    }
}

void AnalogSynthesizer::updateEnvelope (Envelope& env)
{
    if (env.phase == 0.0f) // Attack
    {
        env.value += env.attackRate;
        if (env.value >= 1.0f)
        {
            env.value = 1.0f;
            env.phase = 1.0f; // Decay
        }
    }
    else if (env.phase == 1.0f) // Decay
    {
        env.value -= env.decayRate;
        if (env.value <= env.sustainLevel)
        {
            env.value = env.sustainLevel;
            env.phase = 2.0f; // Sustain
        }
    }
    else if (env.phase == 2.0f) // Sustain
    {
        if (!env.isKeyOn)
            env.phase = 3.0f; // Release
    }
    else if (env.phase == 3.0f) // Release
    {
        env.value -= env.releaseRate;
        if (env.value <= 0.0f)
        {
            env.value = 0.0f;
        }
    }
}

float AnalogSynthesizer::generateLFO()
{
    float phase = std::fmod (lfoPhase, juce::MathConstants<float>::twoPi);
    
    switch (lfoWaveform)
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

float AnalogSynthesizer::processChorus (float input)
{
    if (!chorusEnabled)
        return input;
    
    float output = input;
    
    for (int i = 0; i < 3; ++i)
    {
        float modAmount = std::sin (chorusPhase[i]) * chorusDepth;
        float baseDelay = static_cast<float> (sampleRate * 0.05f); // 50ms base delay
        float delayTime = baseDelay + (i * baseDelay * 0.1f) + modAmount * (baseDelay * 0.2f);
        delayTime = juce::jlimit (1.0f, static_cast<float> (sampleRate * 0.1f), delayTime);
        chorusDelay[i].setDelay (delayTime);
        
        float delayed = chorusDelay[i].popSample (0);
        chorusDelay[i].pushSample (0, input);
        
        output += delayed * 0.33f; // Mix in delayed signal
        
        chorusPhase[i] += chorusRate / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;
        chorusPhase[i] = std::fmod (chorusPhase[i], juce::MathConstants<float>::twoPi);
    }
    
    return output * 0.5f; // Scale down to prevent clipping
}

void AnalogSynthesizer::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    // Update LFO
    float lfoIncrement = lfoRate / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;
    
    for (int sample = startSample; sample < startSample + numSamples; ++sample)
    {
        float lfoValue = generateLFO() * lfoAmount;
        
        float output = 0.0f;
        
        // Process all active voices
        for (int v = 0; v < maxVoices; ++v)
        {
            if (voices[v].isActive)
            {
                Voice& voice = voices[v];
                
                // Update envelopes
                updateEnvelope (voice.ampEnvelope);
                updateEnvelope (voice.filterEnvelope);
                
                // Apply pitch bend
                float freqMultiplier = std::pow (2.0f, voice.pitchBend / 12.0f);
                float baseFreq = midiNoteToFrequency (voice.midiNote) * freqMultiplier;
                
                // Generate oscillator outputs
                float oscOutput = 0.0f;
                for (int osc = 0; osc < 2; ++osc)
                {
                    float freq = baseFreq * std::pow (2.0f, voice.oscillators[osc].octave + voice.oscillators[osc].tune / 12.0f);
                    
                    // Apply LFO to pitch if needed
                    if (lfoDestination == 1 || lfoDestination == 2)
                        freq *= (1.0f + lfoValue * 0.1f);
                    
                    voice.oscillators[osc].phaseIncrement = freq / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;
                    voice.oscillators[osc].phase += voice.oscillators[osc].phaseIncrement;
                    
                    // Wrap phase smoothly using fmod to prevent discontinuities
                    voice.oscillators[osc].phase = std::fmod (voice.oscillators[osc].phase + juce::MathConstants<float>::twoPi, 
                                                              juce::MathConstants<float>::twoPi);
                    
                    oscOutput += generateWaveform (voice.oscillators[osc]) * voice.oscillators[osc].level;
                }
                
                // Add sub oscillator (Juno style)
                if (subOscEnabled)
                {
                    float subFreq = baseFreq * std::pow (2.0f, subOscOctave);
                    subOscPhase += subFreq / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;
                    subOscPhase = std::fmod (subOscPhase, juce::MathConstants<float>::twoPi);
                    oscOutput += std::sin (subOscPhase) * 0.3f; // Sub osc is sine, quieter
                }
                
                // Apply filter envelope
                float filterMod = voice.filterEnvelope.value * filterEnvAmount;
                float cutoff = voice.filterCutoff * (1.0f + filterMod);
                
                // Apply LFO to filter if needed
                if (lfoDestination == 0 || lfoDestination == 2)
                    cutoff *= (1.0f + lfoValue * 0.5f);
                
                cutoff = juce::jlimit (20.0f, 20000.0f, cutoff);
                
                // Simple one-pole filter processing (avoids per-sample temp buffer creation)
                // Use a simpler approach: apply a soft low-pass directly to reduce artifacts
                float qFactor = 0.1f + (filterResonance * 2.5f); // Lower max Q to prevent instability
                qFactor = juce::jlimit (0.1f, 2.5f, qFactor);
                
                // Simple one-pole filter for smooth sound (less artifacts than full IIR per sample)
                float filterAlpha = juce::jlimit (0.0f, 1.0f, cutoff / (cutoff + static_cast<float>(sampleRate) / (2.0f * qFactor)));
                float filteredOutput = voice.lastFiltered * (1.0f - filterAlpha) + oscOutput * filterAlpha;
                voice.lastFiltered = filteredOutput;
                
                // Apply amplitude envelope with smooth interpolation - keep volume high
                float envelopeValue = voice.ampEnvelope.value;
                
                // Smooth envelope transitions to prevent clicks WITHOUT reducing volume
                // Interpolate between last and current envelope value smoothly
                float smoothedEnv = envelopeValue;
                if (std::abs (envelopeValue - voice.lastEnvValue) > 0.001f)
                {
                    // Fast but smooth transition - doesn't reduce volume, just smooths jumps
                    float envDiff = envelopeValue - voice.lastEnvValue;
                    smoothedEnv = voice.lastEnvValue + envDiff * 0.3f; // Fast smoothing, minimal volume reduction
                }
                
                // Much louder to match Sampler - increased significantly
                float voiceOutput = filteredOutput * smoothedEnv * voice.velocity * 3.5f;
                voice.lastEnvValue = smoothedEnv;
                
                // Process chorus (keep volume high)
                if (chorusEnabled)
                {
                    voiceOutput = processChorus (voiceOutput * 0.9f) * 1.1f; // Less reduction, keeps more volume
                }
                
                // Prevent clipping but allow higher headroom
                voiceOutput = juce::jlimit (-1.2f, 1.2f, voiceOutput);
                
                // Check if voice should be deactivated (fully released)
                if (envelopeValue <= 0.001f && voice.ampEnvelope.phase == 3.0f)
                {
                    voice.isActive = false;
                    voice.ampEnvelope.value = 0.0f;
                    voice.lastFiltered = 0.0f;
                    voice.lastEnvValue = 0.0f;
                }
                else
                {
                    output += voiceOutput;
                }
            }
        }
        
        // Update LFO phase
        lfoPhase += lfoIncrement;
        lfoPhase = std::fmod (lfoPhase, juce::MathConstants<float>::twoPi);
        
        // Final output - much louder to match Sampler
        output = juce::jlimit (-1.0f, 1.0f, output * 1.5f); // Significant boost
        
        // Soft saturation for warmth - prevents harsh clipping while keeping volume
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
}

// Parameter setters
void AnalogSynthesizer::setOscillatorWaveform (int oscIndex, int waveform)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].oscillators[oscIndex].waveform = juce::jlimit (0, 3, waveform);
    }
}

void AnalogSynthesizer::setOscillatorOctave (int oscIndex, int octave)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].oscillators[oscIndex].octave = juce::jlimit (-2, 2, octave);
    }
}

void AnalogSynthesizer::setOscillatorTune (int oscIndex, float semitones)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].oscillators[oscIndex].tune = juce::jlimit (-12.0f, 12.0f, semitones);
    }
}

void AnalogSynthesizer::setOscillatorLevel (int oscIndex, float level)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].oscillators[oscIndex].level = juce::jlimit (0.0f, 1.0f, level);
    }
}

void AnalogSynthesizer::setPulseWidth (int oscIndex, float pulseWidth)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].oscillators[oscIndex].pulseWidth = juce::jlimit (0.0f, 1.0f, pulseWidth);
    }
}

void AnalogSynthesizer::setFilterCutoff (float cutoffHz)
{
    filterCutoffBase = juce::jlimit (20.0f, 20000.0f, cutoffHz);
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive)
        {
            float keyTracking = (voices[v].midiNote - 60) / 12.0f * filterKeyTracking;
            voices[v].filterCutoff = filterCutoffBase * std::pow (2.0f, keyTracking);
        }
    }
    updateFilter();
}

void AnalogSynthesizer::setFilterResonance (float resonance)
{
    // Map 0.0-1.0 resonance to Q factor 0.1 to 5.0 (prevent instability)
    filterResonance = juce::jlimit (0.0f, 1.0f, resonance);
    updateFilter();
}

void AnalogSynthesizer::updateFilter()
{
    // Map resonance (0.0-1.0) to Q factor (0.1 to 5.0)
    // Higher resonance = higher Q, but cap at 5.0 to prevent instability
    float qFactor = 0.1f + (filterResonance * 4.9f);
    *filter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (
        sampleRate, filterCutoffBase, qFactor);
}

void AnalogSynthesizer::setFilterEnvelopeAmount (float amount)
{
    filterEnvAmount = juce::jlimit (-1.0f, 1.0f, amount);
}

void AnalogSynthesizer::setFilterKeyboardTracking (float tracking)
{
    filterKeyTracking = juce::jlimit (0.0f, 1.0f, tracking);
}

void AnalogSynthesizer::setEnvelopeAttack (float attackSeconds)
{
    float rate = 1.0f / (attackSeconds * static_cast<float> (sampleRate));
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].ampEnvelope.attackRate = rate;
    }
}

void AnalogSynthesizer::setEnvelopeDecay (float decaySeconds)
{
    float rate = 1.0f / (decaySeconds * static_cast<float> (sampleRate));
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].ampEnvelope.decayRate = rate;
    }
}

void AnalogSynthesizer::setEnvelopeSustain (float sustainLevel)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].ampEnvelope.sustainLevel = juce::jlimit (0.0f, 1.0f, sustainLevel);
    }
}

void AnalogSynthesizer::setEnvelopeRelease (float releaseSeconds)
{
    // Ensure minimum release time to prevent instant release
    float safeRelease = juce::jmax (0.01f, releaseSeconds);
    float rate = 1.0f / (safeRelease * static_cast<float> (sampleRate));
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].ampEnvelope.releaseRate = rate;
    }
}

void AnalogSynthesizer::setFilterEnvelopeAttack (float attackSeconds)
{
    float rate = 1.0f / (attackSeconds * static_cast<float> (sampleRate));
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].filterEnvelope.attackRate = rate;
    }
}

void AnalogSynthesizer::setFilterEnvelopeDecay (float decaySeconds)
{
    float rate = 1.0f / (decaySeconds * static_cast<float> (sampleRate));
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].filterEnvelope.decayRate = rate;
    }
}

void AnalogSynthesizer::setFilterEnvelopeSustain (float sustainLevel)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].filterEnvelope.sustainLevel = juce::jlimit (0.0f, 1.0f, sustainLevel);
    }
}

void AnalogSynthesizer::setFilterEnvelopeRelease (float releaseSeconds)
{
    float rate = 1.0f / (releaseSeconds * static_cast<float> (sampleRate));
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].filterEnvelope.releaseRate = rate;
    }
}

void AnalogSynthesizer::setLFORate (float rateHz)
{
    lfoRate = juce::jlimit (0.1f, 20.0f, rateHz);
}

void AnalogSynthesizer::setLFOAmount (float amount)
{
    lfoAmount = juce::jlimit (0.0f, 1.0f, amount);
}

void AnalogSynthesizer::setLFOWaveform (int waveform)
{
    lfoWaveform = juce::jlimit (0, 3, waveform);
}

void AnalogSynthesizer::setLFODestination (int destination)
{
    lfoDestination = juce::jlimit (0, 2, destination);
}

void AnalogSynthesizer::setChorusEnabled (bool enabled)
{
    chorusEnabled = enabled;
}

void AnalogSynthesizer::setChorusRate (float rateHz)
{
    chorusRate = juce::jlimit (0.1f, 5.0f, rateHz);
}

void AnalogSynthesizer::setChorusDepth (float depth)
{
    chorusDepth = juce::jlimit (0.0f, 1.0f, depth);
}

void AnalogSynthesizer::setSubOscillatorEnabled (bool enabled)
{
    subOscEnabled = enabled;
}

void AnalogSynthesizer::setSubOscillatorOctave (int octave)
{
    subOscOctave = (octave == -2) ? -2 : -1;
}

void AnalogSynthesizer::setPitchBend (float semitones)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive)
            voices[v].pitchBend = semitones;
    }
}

