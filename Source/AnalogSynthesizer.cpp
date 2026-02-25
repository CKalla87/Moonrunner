/*
  ==============================================================================

    AnalogSynthesizer.cpp
    Van Halen "Jump" - Oberheim OB-Xa brass synth (1:1 recreation)

  ==============================================================================
*/

#include "AnalogSynthesizer.h"

//==============================================================================
AnalogSynthesizer::AnalogSynthesizer()
{
}

void AnalogSynthesizer::prepare (double sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (juce::jmax (1, samplesPerBlock));
    spec.numChannels = 1;

    for (int v = 0; v < maxVoices; ++v)
    {
        voiceFilters[v].reset();
        voiceFilters[v].setMode (juce::dsp::LadderFilterMode::LPF24);
        voiceFilters[v].prepare (spec);
        voiceFilters[v].setCutoffFrequencyHz (filterCutoffBase);
        voiceFilters[v].setResonance (filterResonance);
        voiceFilters[v].setDrive (1.2f);
    }

    filterBuffer.setSize (1, 1);

    const int maxDelaySamples = static_cast<int> (sampleRate * 0.03f);
    for (int i = 0; i < 2; ++i)
    {
        chorusDelay[i].prepare (spec);
        chorusDelay[i].setMaximumDelayInSamples (maxDelaySamples);
    }

    float ampAttackRate = 1.0f / (ampAttackSec * static_cast<float> (sampleRate));
    float ampDecayRate = 1.0f / (ampDecaySec * static_cast<float> (sampleRate));
    float ampReleaseRate = 1.0f / (juce::jmax (0.01f, ampReleaseSec) * static_cast<float> (sampleRate));
    float filtAttackRate = 1.0f / (filtAttackSec * static_cast<float> (sampleRate));
    float filtDecayRate = 1.0f / (filtDecaySec * static_cast<float> (sampleRate));
    float filtReleaseRate = 1.0f / (juce::jmax (0.01f, filtReleaseSec) * static_cast<float> (sampleRate));

    for (int v = 0; v < maxVoices; ++v)
    {
        voices[v].ampEnv.attackRate = ampAttackRate;
        voices[v].ampEnv.decayRate = ampDecayRate;
        voices[v].ampEnv.sustainLevel = ampSustain;
        voices[v].ampEnv.releaseRate = ampReleaseRate;
        voices[v].filterEnv.attackRate = filtAttackRate;
        voices[v].filterEnv.decayRate = filtDecayRate;
        voices[v].filterEnv.sustainLevel = filtSustain;
        voices[v].filterEnv.releaseRate = filtReleaseRate;
        voices[v].isActive = false;
        voices[v].phase[0] = 0.0f;
        voices[v].phase[1] = 0.0f;
        voices[v].ampEnv.value = 0.0f;
        voices[v].ampEnv.phase = 0.0f;
        voices[v].filterEnv.value = 0.0f;
        voices[v].filterEnv.phase = 0.0f;
    }

    chorusPhase[0] = 0.0f;
    chorusPhase[1] = juce::MathConstants<float>::pi;
}

void AnalogSynthesizer::reset()
{
    allNotesOff();
    for (int v = 0; v < maxVoices; ++v)
        voiceFilters[v].reset();
}

float AnalogSynthesizer::midiNoteToFrequency (int midiNote)
{
    return 440.0f * std::pow (2.0f, (midiNote - 69) / 12.0f);
}

float AnalogSynthesizer::saw (float phase)
{
    phase = std::fmod (phase + juce::MathConstants<float>::twoPi, juce::MathConstants<float>::twoPi);
    return 2.0f * (phase / juce::MathConstants<float>::twoPi) - 1.0f;
}

void AnalogSynthesizer::updateEnvelope (JumpEnvelope& env)
{
    if (env.phase == 0.0f)
    {
        env.value += env.attackRate;
        if (env.value >= 1.0f)
        {
            env.value = 1.0f;
            env.phase = 1.0f;
        }
    }
    else if (env.phase == 1.0f)
    {
        env.value -= env.decayRate;
        if (env.value <= env.sustainLevel)
        {
            env.value = env.sustainLevel;
            env.phase = 2.0f;
        }
    }
    else if (env.phase == 2.0f)
    {
        if (!env.isKeyOn)
            env.phase = 3.0f;
    }
    else if (env.phase == 3.0f)
    {
        env.value -= env.releaseRate;
        if (env.value <= 0.0f)
            env.value = 0.0f;
    }
}

int AnalogSynthesizer::findFreeVoice()
{
    for (int v = 0; v < maxVoices; ++v)
        if (!voices[v].isActive)
            return v;

    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].ampEnv.value < 0.001f && voices[v].ampEnv.phase == 3.0f)
        {
            voices[v].isActive = false;
            return v;
        }
    }

    int steal = 0;
    float lowest = 1.0f;
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].ampEnv.phase == 3.0f && voices[v].ampEnv.value < lowest)
        {
            lowest = voices[v].ampEnv.value;
            steal = v;
        }
    }
    if (lowest >= 0.99f)
    {
        for (int v = 0; v < maxVoices; ++v)
        {
            if (voices[v].ampEnv.value < lowest)
            {
                lowest = voices[v].ampEnv.value;
                steal = v;
            }
        }
    }
    voices[steal].ampEnv.isKeyOn = false;
    voices[steal].ampEnv.phase = 3.0f;
    voices[steal].filterEnv.isKeyOn = false;
    voices[steal].filterEnv.phase = 3.0f;
    return steal;
}

int AnalogSynthesizer::findVoiceForNote (int midiNote)
{
    for (int v = 0; v < maxVoices; ++v)
        if (voices[v].isActive && voices[v].midiNote == midiNote)
            return v;
    return -1;
}

void AnalogSynthesizer::noteOn (int midiNoteNumber, float velocity)
{
    int existing = findVoiceForNote (midiNoteNumber);
    if (existing >= 0)
    {
        voices[existing].ampEnv.isKeyOn = false;
        voices[existing].ampEnv.phase = 3.0f;
        voices[existing].filterEnv.isKeyOn = false;
        voices[existing].filterEnv.phase = 3.0f;
    }

    int v = findFreeVoice();
    if (v < 0 || v >= maxVoices)
        return;

    JumpVoice& voice = voices[v];
    voice.isActive = true;
    voice.midiNote = midiNoteNumber;
    voice.baseFreq = midiNoteToFrequency (midiNoteNumber);
    voice.velocity = velocity;
    voice.pitchBend = 0.0f;
    voice.phase[0] = 0.0f;
    voice.phase[1] = 0.0f;

    float freq0 = voice.baseFreq;
    float freq1 = voice.baseFreq * std::pow (2.0f, osc2DetuneCents / 1200.0f);
    voice.phaseIncrement[0] = freq0 / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;
    voice.phaseIncrement[1] = freq1 / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;

    voice.ampEnv.value = 0.0f;
    voice.ampEnv.phase = 0.0f;
    voice.ampEnv.isKeyOn = true;
    voice.filterEnv.value = 0.0f;
    voice.filterEnv.phase = 0.0f;
    voice.filterEnv.isKeyOn = true;
}

void AnalogSynthesizer::noteOff (int midiNoteNumber)
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive && voices[v].midiNote == midiNoteNumber)
        {
            voices[v].ampEnv.isKeyOn = false;
            if (voices[v].ampEnv.phase == 2.0f)
                voices[v].ampEnv.value = voices[v].ampEnv.sustainLevel;
            voices[v].ampEnv.phase = 3.0f;
            voices[v].filterEnv.isKeyOn = false;
            if (voices[v].filterEnv.phase == 2.0f)
                voices[v].filterEnv.value = voices[v].filterEnv.sustainLevel;
            voices[v].filterEnv.phase = 3.0f;
        }
    }
}

void AnalogSynthesizer::allNotesOff()
{
    for (int v = 0; v < maxVoices; ++v)
    {
        if (voices[v].isActive)
            noteOff (voices[v].midiNote);
    }
}

void AnalogSynthesizer::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    const float chorusInc = chorusRateHz / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;
    const float baseDelaySamples = static_cast<float> (sampleRate * 0.02f);

    for (int sample = startSample; sample < startSample + numSamples; ++sample)
    {
        float mix = 0.0f;

        for (int v = 0; v < maxVoices; ++v)
        {
            if (!voices[v].isActive)
                continue;

            JumpVoice& voice = voices[v];
            updateEnvelope (voice.ampEnv);
            updateEnvelope (voice.filterEnv);

            float freqMul = std::pow (2.0f, voice.pitchBend / 12.0f);
            float f0 = midiNoteToFrequency (voice.midiNote) * freqMul;
            float f1 = f0 * std::pow (2.0f, osc2DetuneCents / 1200.0f);
            voice.phaseIncrement[0] = f0 / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;
            voice.phaseIncrement[1] = f1 / static_cast<float> (sampleRate) * juce::MathConstants<float>::twoPi;

            voice.phase[0] += voice.phaseIncrement[0];
            voice.phase[1] += voice.phaseIncrement[1];
            voice.phase[0] = std::fmod (voice.phase[0] + juce::MathConstants<float>::twoPi, juce::MathConstants<float>::twoPi);
            voice.phase[1] = std::fmod (voice.phase[1] + juce::MathConstants<float>::twoPi, juce::MathConstants<float>::twoPi);

            float oscOut = saw (voice.phase[0]) * osc1Level + saw (voice.phase[1]) * osc2Level;
            oscOut *= 0.5f;

            float keyTrack = (voice.midiNote - 60) / 12.0f * filterKeyTracking;
            float baseCutoff = filterCutoffBase * std::pow (2.0f, keyTrack);
            float cutoff = baseCutoff * (1.0f + voice.filterEnv.value * filterEnvAmount);
            cutoff = juce::jlimit (80.0f, 18000.0f, cutoff);

            voiceFilters[v].setCutoffFrequencyHz (cutoff);
            filterBuffer.setSample (0, 0, oscOut);
            juce::dsp::AudioBlock<float> block (filterBuffer);
            juce::dsp::ProcessContextReplacing<float> ctx (block);
            voiceFilters[v].process (ctx);
            float filtered = filterBuffer.getSample (0, 0);

            float out = filtered * voice.ampEnv.value * voice.velocity * 2.2f;

            if (voice.ampEnv.value <= 0.001f && voice.ampEnv.phase == 3.0f)
            {
                voice.isActive = false;
            }
            else
            {
                mix += out;
            }
        }

        mix = juce::jlimit (-1.5f, 1.5f, mix);

        if (std::abs (mix) > 0.85f)
        {
            float s = mix > 0.0f ? 1.0f : -1.0f;
            mix = s * (0.85f + 0.15f * std::tanh ((std::abs (mix) - 0.85f) * 8.0f));
        }

        chorusPhase[0] += chorusInc;
        chorusPhase[1] += chorusInc;
        chorusPhase[0] = std::fmod (chorusPhase[0], juce::MathConstants<float>::twoPi);
        chorusPhase[1] = std::fmod (chorusPhase[1], juce::MathConstants<float>::twoPi);

        float d0 = baseDelaySamples + std::sin (chorusPhase[0]) * chorusDepth * baseDelaySamples * 0.5f;
        float d1 = baseDelaySamples * 1.1f + std::sin (chorusPhase[1]) * chorusDepth * baseDelaySamples * 0.5f;
        d0 = juce::jlimit (1.0f, static_cast<float> (chorusDelay[0].getMaximumDelayInSamples()), d0);
        d1 = juce::jlimit (1.0f, static_cast<float> (chorusDelay[1].getMaximumDelayInSamples()), d1);
        chorusDelay[0].setDelay (d0);
        chorusDelay[1].setDelay (d1);

        float wet0 = chorusDelay[0].popSample (0);
        float wet1 = chorusDelay[1].popSample (0);
        chorusDelay[0].pushSample (0, mix);
        chorusDelay[1].pushSample (0, mix);

        float stereoL = mix * 0.6f + wet0 * 0.4f;
        float stereoR = mix * 0.6f + wet1 * 0.4f;
        mix = (stereoL + stereoR) * 0.5f;
        mix = juce::jlimit (-1.0f, 1.0f, mix * 1.1f);

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addSample (ch, sample, mix);
    }
}

void AnalogSynthesizer::setOscillatorWaveform (int, int) {}
void AnalogSynthesizer::setOscillatorOctave (int, int) {}
void AnalogSynthesizer::setOscillatorTune (int, float) {}
void AnalogSynthesizer::setOscillatorLevel (int, float) {}
void AnalogSynthesizer::setPulseWidth (int, float) {}
void AnalogSynthesizer::setFilterCutoff (float) {}
void AnalogSynthesizer::setFilterResonance (float) {}
void AnalogSynthesizer::setFilterEnvelopeAmount (float) {}
void AnalogSynthesizer::setFilterKeyboardTracking (float) {}
void AnalogSynthesizer::setEnvelopeAttack (float) {}
void AnalogSynthesizer::setEnvelopeDecay (float) {}
void AnalogSynthesizer::setEnvelopeSustain (float) {}
void AnalogSynthesizer::setEnvelopeRelease (float) {}
void AnalogSynthesizer::setFilterEnvelopeAttack (float) {}
void AnalogSynthesizer::setFilterEnvelopeDecay (float) {}
void AnalogSynthesizer::setFilterEnvelopeSustain (float) {}
void AnalogSynthesizer::setFilterEnvelopeRelease (float) {}
void AnalogSynthesizer::setLFORate (float) {}
void AnalogSynthesizer::setLFOAmount (float) {}
void AnalogSynthesizer::setLFOWaveform (int) {}
void AnalogSynthesizer::setLFODestination (int) {}
void AnalogSynthesizer::setChorusEnabled (bool) {}
void AnalogSynthesizer::setChorusRate (float) {}
void AnalogSynthesizer::setChorusDepth (float) {}
void AnalogSynthesizer::setSubOscillatorEnabled (bool) {}
void AnalogSynthesizer::setSubOscillatorOctave (int) {}

void AnalogSynthesizer::setPitchBend (float semitones)
{
    for (int v = 0; v < maxVoices; ++v)
        if (voices[v].isActive)
            voices[v].pitchBend = semitones;
}
