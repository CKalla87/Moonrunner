/*
  ==============================================================================

    SamplerEngine.cpp
    Industrial Bass Synth - NIN "Head Like a Hole" style

  ==============================================================================
*/

#include "SamplerEngine.h"

namespace
{
    // Detune amounts in cents for thick industrial unison (NIN Head Like a Hole)
    constexpr float kDetuneCents[4] = { -25.0f, -10.0f, 10.0f, 25.0f };

    inline float saw (float phase)
    {
        return 2.0f * phase - 1.0f;
    }

    // Heavy NIN-style distortion - hard clipping + waveshaping
    inline float distort (float x)
    {
        x *= 4.5f;  // Drive into saturation
        return std::tanh (x) * 0.95f;
    }
}

//==============================================================================
SamplerEngine::SamplerEngine()
{
}

void SamplerEngine::prepare (double sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;

    currentAttackTime = 0.003f;
    currentReleaseTime = 0.4f;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 2;

    ladderFilter.reset();
    ladderFilter.setMode (juce::dsp::LadderFilterMode::LPF24);
    ladderFilter.prepare (spec);
    ladderFilter.setCutoffFrequencyHz (filterCutoffBase);
    ladderFilter.setResonance (juce::jlimit (0.0f, 0.95f, filterResonanceBase));
    ladderFilter.setDrive (2.0f);  // NIN grit

    filterBuffer.setSize (2, samplesPerBlock);

    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].isActive = false;
        voices[v].envelopeValue = 0.0f;
        voices[v].envelopePhase = 1.0f;
    }
}

void SamplerEngine::reset()
{
    allNotesOff();
    ladderFilter.reset();
}

float SamplerEngine::midiNoteToFrequency (int midiNote)
{
    return 440.0f * std::pow (2.0f, (midiNote - 69) / 12.0f);
}

void SamplerEngine::noteOn (int midiNoteNumber, float velocity)
{
    int existingVoice = findVoiceForNote (midiNoteNumber);
    if (existingVoice != -1)
    {
        voices[existingVoice].isKeyOn = false;
        voices[existingVoice].envelopePhase = 1.0f;
    }

    int voiceIndex = findFreeVoice();
    if (voiceIndex < 0 || voiceIndex >= maxVoices)
        return;

    Voice& voice = voices[voiceIndex];
    voice.isActive = true;
    voice.midiNote = midiNoteNumber;
    voice.frequency = midiNoteToFrequency (midiNoteNumber);
    voice.velocity = velocity;
    voice.pitchBend = 0.0f;
    voice.isKeyOn = true;
    voice.envelopeValue = 0.0f;
    voice.envelopePhase = 0.0f;

    juce::Random r (midiNoteNumber * 31 + static_cast<int> (velocity * 1000));
    for (int i = 0; i < 4; ++i)
        voice.phase[i] = r.nextFloat();
    voice.subPhase = r.nextFloat();

    float safeAttack = juce::jmax (0.001f, currentAttackTime);
    voice.attackRate = 1.0f / (safeAttack * static_cast<float> (sampleRate));
    float safeRelease = juce::jmax (0.01f, currentReleaseTime);
    voice.releaseRate = 1.0f / (safeRelease * static_cast<float> (sampleRate));
}

void SamplerEngine::noteOff (int midiNoteNumber)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive && voices[v].midiNote == midiNoteNumber)
        {
            voices[v].isKeyOn = false;
            voices[v].envelopePhase = 1.0f;
        }
    }
}

void SamplerEngine::allNotesOff()
{
    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].isKeyOn = false;
        voices[v].envelopePhase = 1.0f;
    }
}

int SamplerEngine::findFreeVoice()
{
    for (int v = 0; v < maxVoices; ++v)
        if (!voices[v].isActive)
            return v;

    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].envelopeValue < 0.001f && voices[v].envelopePhase == 1.0f && !voices[v].isKeyOn)
        {
            voices[v].isActive = false;
            return v;
        }
    }

    int stealIndex = 0;
    float lowest = 1.0f;
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].envelopeValue < lowest)
        {
            lowest = voices[v].envelopeValue;
            stealIndex = v;
        }
    }
    voices[stealIndex].isKeyOn = false;
    voices[stealIndex].envelopePhase = 1.0f;
    return stealIndex;
}

int SamplerEngine::findVoiceForNote (int midiNote)
{
    for (int v = 0; v < maxVoices; ++v)
        if (voices[v].isActive && voices[v].midiNote == midiNote)
            return v;
    return -1;
}

void SamplerEngine::updateEnvelope (Voice& voice)
{
    if (voice.envelopePhase == 0.0f)
    {
        voice.envelopeValue += voice.attackRate;
        if (voice.envelopeValue >= 1.0f)
            voice.envelopeValue = 1.0f;
    }
    else
    {
        voice.envelopeValue -= voice.releaseRate;
        if (voice.envelopeValue <= 0.0f)
        {
            voice.envelopeValue = 0.0f;
            voice.isActive = false;
        }
    }
}

float SamplerEngine::renderVoice (Voice& voice)
{
    float pitchMult = std::pow (2.0f, voice.pitchBend / 12.0f);
    float freq = voice.frequency * pitchMult;
    float phaseInc = static_cast<float> (freq / sampleRate);
    float subInc = phaseInc * 0.5f;

    float sample = 0.0f;

    for (int i = 0; i < 4; ++i)
    {
        float ratio = std::pow (2.0f, kDetuneCents[i] / 1200.0f);
        float inc = phaseInc * ratio;
        sample += saw (voice.phase[i]);
        voice.phase[i] += inc;
        if (voice.phase[i] >= 1.0f) voice.phase[i] -= 1.0f;
        if (voice.phase[i] < 0.0f) voice.phase[i] += 1.0f;
    }

    sample *= 0.28f;  // Thicker detuned blend
    sample += 0.48f * saw (voice.subPhase);  // Heavy sub for weight
    voice.subPhase += subInc;
    if (voice.subPhase >= 1.0f) voice.subPhase -= 1.0f;
    if (voice.subPhase < 0.0f) voice.subPhase += 1.0f;

    sample = distort (sample);
    return sample * voice.envelopeValue * voice.velocity;
}

void SamplerEngine::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    outputBuffer.clear (startSample, numSamples);

    for (int s = 0; s < numSamples; ++s)
    {
        float output = 0.0f;

        for (int v = 0; v < maxVoices; ++v)
        {
            if (voices[v].isActive)
            {
                Voice& voice = voices[v];
                updateEnvelope (voice);
                output += renderVoice (voice);
            }
        }

        output = juce::jlimit (-0.95f, 0.95f, output);
        output = distort (output * 1.5f);  // More drive on master

        filterBuffer.setSample (0, s, output);
        filterBuffer.setSample (1, s, output);
    }

    juce::dsp::AudioBlock<float> block (filterBuffer);
    auto subBlock = block.getSubBlock (0, static_cast<size_t> (numSamples));
    juce::dsp::ProcessContextReplacing<float> context (subBlock);
    ladderFilter.process (context);

    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
    {
        if (ch < 2)
            outputBuffer.addFrom (ch, startSample, filterBuffer, ch, 0, numSamples, 0.9f);
        else
            outputBuffer.addFrom (ch, startSample, filterBuffer, 0, 0, numSamples, 0.9f);
    }
}

void SamplerEngine::setAttack (float attackSeconds)
{
    currentAttackTime = juce::jmax (0.001f, attackSeconds);
    float rate = 1.0f / (currentAttackTime * static_cast<float> (sampleRate));
    for (int v = 0; v < maxVoices; ++v)
        voices[v].attackRate = rate;
}

void SamplerEngine::setRelease (float releaseSeconds)
{
    currentReleaseTime = juce::jmax (0.01f, releaseSeconds);
    float rate = 1.0f / (currentReleaseTime * static_cast<float> (sampleRate));
    for (int v = 0; v < maxVoices; ++v)
        voices[v].releaseRate = rate;
}

void SamplerEngine::setFilterCutoff (float cutoffHz)
{
    filterCutoffBase = juce::jlimit (80.0f, 16000.0f, cutoffHz);
    ladderFilter.setCutoffFrequencyHz (filterCutoffBase);
}

void SamplerEngine::setFilterResonance (float resonance)
{
    filterResonanceBase = juce::jlimit (0.0f, 0.95f, resonance);
    ladderFilter.setResonance (filterResonanceBase);
}

void SamplerEngine::setPitchBend (float semitones)
{
    for (int v = 0; v < maxVoices; ++v)
        if (voices[v].isActive)
            voices[v].pitchBend = semitones;
}
