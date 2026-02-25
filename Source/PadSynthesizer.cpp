/*
  ==============================================================================

    PadSynthesizer.cpp
    Super-wide pad: PolyBLEP oscillators, drift, ladder, chorus, reverb

  ==============================================================================
*/

#include "PadSynthesizer.h"

namespace
{
    static inline float softClip (float x) { return std::tanh (x); }
    static inline float lerp (float a, float b, float t) { return a + (b - a) * t; }

    static inline float polyBlep (float t, float dt)
    {
        if (t < dt) { float x = t / dt; return x + x - x * x - 1.0f; }
        if (t > 1.0f - dt) { float x = (t - 1.0f) / dt; return x * x + x + x + 1.0f; }
        return 0.0f;
    }
}

//==============================================================================
struct PolyBlepOsc
{
    enum class Type { Saw, Pulse, Tri };

    void prepare (double sr)
    {
        sampleRate = sr;
        phase = 0.0f;
        setFrequency (110.0f);
    }

    void reset (float newPhase = 0.0f) { phase = newPhase; }

    void setFrequency (float hz)
    {
        frequency = hz;
        phaseInc = (float) (frequency / (float) sampleRate);
    }

    void setType (Type t) { type = t; }
    void setPulseWidth (float pw) { pulseWidth = juce::jlimit (0.05f, 0.95f, pw); }

    float process()
    {
        float t = phase;
        float dt = phaseInc;
        float y = 0.0f;

        switch (type)
        {
            case Type::Saw:
                y = 2.0f * t - 1.0f;
                y -= polyBlep (t, dt);
                break;
            case Type::Pulse:
                y = (t < pulseWidth) ? 1.0f : -1.0f;
                y += polyBlep (t, dt);
                { float t2 = std::fmod (t - pulseWidth + 1.0f, 1.0f); y -= polyBlep (t2, dt); }
                break;
            case Type::Tri:
            {
                float sq = (t < 0.5f) ? 1.0f : -1.0f;
                sq += polyBlep (t, dt);
                float t2 = std::fmod (t - 0.5f + 1.0f, 1.0f);
                sq -= polyBlep (t2, dt);
                integrator = 0.995f * integrator + 0.005f * sq;
                y = integrator;
            }
            break;
        }

        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;
        return y;
    }

    double sampleRate = 44100.0;
    float frequency = 110.0f;
    float phase = 0.0f;
    float phaseInc = 0.0f;
    float pulseWidth = 0.4f;
    Type type = Type::Saw;
    float integrator = 0.0f;
};

//==============================================================================
struct Drift
{
    void prepare (double sr, float updateHz)
    {
        sampleRate = sr;
        updateIntervalSamples = (int) std::max (1.0, sr / updateHz);
        counter = 0;
        valueCents = 0.0f;
        targetCents = 0.0f;
    }

    void setRange (float cents) { rangeCents = std::max (0.0f, cents); }
    void reset() { counter = 0; valueCents = 0.0f; targetCents = 0.0f; }

    float process()
    {
        if (++counter >= updateIntervalSamples)
        {
            counter = 0;
            float step = (rng.nextFloat() * 2.0f - 1.0f) * (rangeCents * 0.25f);
            targetCents = juce::jlimit (-rangeCents, rangeCents, targetCents + step);
        }
        valueCents = lerp (valueCents, targetCents, 0.0025f);
        return valueCents;
    }

    float centsToRatio (float cents) const { return std::pow (2.0f, cents / 1200.0f); }

    double sampleRate = 44100.0;
    int updateIntervalSamples = 22050;
    int counter = 0;
    float rangeCents = 5.0f;
    float valueCents = 0.0f;
    float targetCents = 0.0f;
    juce::Random rng;
};

//==============================================================================
struct MicroDelayStereo
{
    void prepare (double sr, int maxMs = 10)
    {
        sampleRate = sr;
        maxSamples = (int) std::ceil (sr * (maxMs / 1000.0));
        bufferL.assign ((size_t) maxSamples + 2, 0.0f);
        bufferR.assign ((size_t) maxSamples + 2, 0.0f);
        writeIndex = 0;
    }

    void reset()
    {
        std::fill (bufferL.begin(), bufferL.end(), 0.0f);
        std::fill (bufferR.begin(), bufferR.end(), 0.0f);
        writeIndex = 0;
    }

    void push (float inL, float inR)
    {
        bufferL[(size_t) writeIndex] = inL;
        bufferR[(size_t) writeIndex] = inR;
        writeIndex++;
        if (writeIndex >= (int) bufferL.size()) writeIndex = 0;
    }

    void read (float delaySamples, float& outL, float& outR) const
    {
        delaySamples = juce::jlimit (0.0f, (float) (maxSamples - 1), delaySamples);
        float readPos = (float) writeIndex - delaySamples;
        while (readPos < 0.0f) readPos += (float) bufferL.size();
        int i0 = (int) readPos;
        int i1 = (i0 + 1) % (int) bufferL.size();
        float frac = readPos - (float) i0;
        outL = lerp (bufferL[(size_t) i0], bufferL[(size_t) i1], frac);
        outR = lerp (bufferR[(size_t) i0], bufferR[(size_t) i1], frac);
    }

    double sampleRate = 44100.0;
    int maxSamples = 441;
    int writeIndex = 0;
    std::vector<float> bufferL, bufferR;
};

//==============================================================================
class SuperWidePadVoice : public juce::SynthesiserVoice
{
public:
    static constexpr int kUnison = 6;

    void prepare (double sr, int blockSize)
    {
        sampleRate = sr;
        for (int i = 0; i < kUnison; ++i)
        {
            oscSaw1[i].prepare (sr);
            oscSaw2[i].prepare (sr);
            oscPulse[i].prepare (sr);
            subTri[i].prepare (sr);
            oscSaw1[i].setType (PolyBlepOsc::Type::Saw);
            oscSaw2[i].setType (PolyBlepOsc::Type::Saw);
            oscPulse[i].setType (PolyBlepOsc::Type::Pulse);
            subTri[i].setType (PolyBlepOsc::Type::Tri);
            drift1[i].prepare (sr, 2.0f);
            drift2[i].prepare (sr, 2.0f);
            driftP[i].prepare (sr, 2.0f);
            drift1[i].setRange (5.0f);
            drift2[i].setRange (5.0f);
            driftP[i].setRange (3.0f);
        }
        ladder.reset();
        ladder.setMode (juce::dsp::LadderFilterMode::LPF24);
        ladder.prepare ({ sr, (juce::uint32) blockSize, 2 });
        ladderBuffer.setSize (2, 1);
        microDelay.prepare (sr, 10);
        ampAdsr.setSampleRate (sr);
        filtAdsr.setSampleRate (sr);
        lfoPhase1 = 0.0f;
        lfoPhase2 = 0.0f;
    }

    void setParams (const juce::AudioProcessorValueTreeState& apvts)
    {
        auto getFloat = [&] (const char* id, float def)
        {
            auto* p = apvts.getRawParameterValue (id);
            return p ? p->load() : def;
        };

        masterGain = getFloat ("PadGain", 0.2f);
        cutoffBaseHz = getFloat ("PadCutoff", 800.0f);
        resonance = getFloat ("PadRes", 0.15f);
        drive = getFloat ("PadDrive", 1.2f);
        ampA = getFloat ("PadAmpA", 0.18f);
        ampD = getFloat ("PadAmpD", 1.2f);
        ampS = getFloat ("PadAmpS", 0.8f);
        ampR = getFloat ("PadAmpR", 2.5f);
        filA = getFloat ("PadFilA", 0.30f);
        filD = getFloat ("PadFilD", 2.0f);
        filS = getFloat ("PadFilS", 0.5f);
        filR = getFloat ("PadFilR", 3.0f);
        filEnvAmt = getFloat ("PadFilEnvAmt", 0.9f);
        keyTrack = getFloat ("PadKeyTrack", 0.35f);
        pwm = getFloat ("PadPWM", 0.40f);
        pwmLfoAmt = getFloat ("PadPWMLfoAmt", 0.05f);
        unisonDetuneCents = getFloat ("PadUniDetune", 12.0f);
        stereoSpread = getFloat ("PadStereoSpread", 0.9f);
        microDelayMsMax = getFloat ("PadMicroDelayMs", 6.0f);
        lfo1Hz = getFloat ("PadLfo1Hz", 0.08f);
        lfo1Amt = getFloat ("PadLfo1Amt", 0.03f);
        lfo2Hz = getFloat ("PadLfo2Hz", 0.15f);
        lfo2Amt = getFloat ("PadLfo2Amt", 0.05f);

        juce::ADSR::Parameters a;
        a.attack = ampA; a.decay = ampD; a.sustain = ampS; a.release = ampR;
        ampAdsr.setParameters (a);
        juce::ADSR::Parameters f;
        f.attack = filA; f.decay = filD; f.sustain = filS; f.release = filR;
        filtAdsr.setParameters (f);
    }

    bool canPlaySound (juce::SynthesiserSound*) override { return true; }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        noteHz = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        noteNumber = midiNoteNumber;
        level = velocity;
        juce::Random r ((int) (noteNumber * 1337 + (int) (level * 1000)));

        for (int i = 0; i < kUnison; ++i)
        {
            float uniPos = (kUnison == 1) ? 0.0f : (float) i / (float) (kUnison - 1);
            float pan = juce::jmap (uniPos, -stereoSpread, stereoSpread);
            unisonPan[i] = pan;
            float ms = r.nextFloat() * microDelayMsMax;
            unisonDelaySamples[i] = (ms / 1000.0f) * (float) sampleRate;
            drift1[i].reset();
            drift2[i].reset();
            driftP[i].reset();
            oscSaw1[i].reset (r.nextFloat());
            oscSaw2[i].reset (r.nextFloat());
            oscPulse[i].reset (r.nextFloat());
            subTri[i].reset (r.nextFloat());
        }
        ampAdsr.noteOn();
        filtAdsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        ampAdsr.noteOff();
        filtAdsr.noteOff();
        if (!allowTailOff) clearCurrentNote();
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (!isVoiceActive()) return;

        auto* left = outputBuffer.getWritePointer (0);
        auto* right = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer (1) : nullptr;

        for (int s = 0; s < numSamples; ++s)
        {
            float lfo1 = std::sin (2.0f * juce::MathConstants<float>::pi * lfoPhase1);
            float lfo2 = std::sin (2.0f * juce::MathConstants<float>::pi * lfoPhase2);
            lfoPhase1 += lfo1Hz / (float) sampleRate;
            lfoPhase2 += lfo2Hz / (float) sampleRate;
            if (lfoPhase1 >= 1.0f) lfoPhase1 -= 1.0f;
            if (lfoPhase2 >= 1.0f) lfoPhase2 -= 1.0f;

            float ampEnv = ampAdsr.getNextSample();
            float filEnv = filtAdsr.getNextSample();
            float keyTrackFactor = std::pow (noteHz / 261.63f, keyTrack);
            float cutoff = cutoffBaseHz * keyTrackFactor;
            cutoff *= (1.0f + filEnvAmt * filEnv);
            cutoff *= (1.0f + lfo1Amt * lfo1);
            cutoff = juce::jlimit (40.0f, 18000.0f, cutoff);
            ladder.setCutoffFrequencyHz (cutoff);
            ladder.setResonance (resonance);

            float rawL = 0.0f, rawR = 0.0f;
            for (int i = 0; i < kUnison; ++i)
            {
                float uniPos = (kUnison == 1) ? 0.0f : (float) i / (float) (kUnison - 1);
                float det = juce::jmap (uniPos, -unisonDetuneCents, unisonDetuneCents);
                float cents1 = det - 6.0f + drift1[i].process();
                float cents2 = det + 6.0f + drift2[i].process();
                float centsP = det + 2.0f + driftP[i].process();
                float f1 = noteHz * drift1[i].centsToRatio (cents1);
                float f2 = noteHz * drift2[i].centsToRatio (cents2);
                float fp = noteHz * driftP[i].centsToRatio (centsP);
                oscSaw1[i].setFrequency (f1);
                oscSaw2[i].setFrequency (f2);
                oscPulse[i].setFrequency (fp);
                float pwmMod = pwm + (pwmLfoAmt * lfo2Amt * lfo2);
                oscPulse[i].setPulseWidth (pwmMod);

                float sig = 0.50f * oscSaw1[i].process() + 0.40f * oscSaw2[i].process()
                          + 0.10f * oscPulse[i].process() + 0.10f * (0.5f * subTri[i].process());
                sig = softClip (sig * drive);

                float pan = unisonPan[i];
                float gL = std::cos (0.5f * juce::MathConstants<float>::pi * (pan + 1.0f) * 0.5f);
                float gR = std::sin (0.5f * juce::MathConstants<float>::pi * (pan + 1.0f) * 0.5f);
                rawL += sig * gL;
                rawR += sig * gR;
            }
            microDelay.push (rawL, rawR);

            float avgDelay = 0.0f;
            for (int i = 0; i < kUnison; ++i) avgDelay += unisonDelaySamples[i];
            avgDelay /= (float) kUnison;
            float dL = 0.0f, dR = 0.0f;
            microDelay.read (avgDelay, dL, dR);
            float stereoL = rawL * 0.82f + dL * 0.18f;
            float stereoR = rawR * 0.82f + dR * 0.18f;

            ladderBuffer.setSample (0, 0, stereoL);
            ladderBuffer.setSample (1, 0, stereoR);
            juce::dsp::AudioBlock<float> ladderBlock (ladderBuffer);
            juce::dsp::ProcessContextReplacing<float> ladderCtx (ladderBlock);
            ladder.process (ladderCtx);
            float fL = ladderBuffer.getSample (0, 0);
            float fR = ladderBuffer.getSample (1, 0);
            float outL = fL * ampEnv * level * masterGain;
            float outR = fR * ampEnv * level * masterGain;

            left[startSample + s] += outL;
            if (right) right[startSample + s] += outR;

            if (!ampAdsr.isActive()) { clearCurrentNote(); break; }
        }
    }

private:
    double sampleRate = 44100.0;
    float noteHz = 110.0f;
    int noteNumber = 60;
    float level = 0.8f;
    float masterGain = 0.2f;
    float cutoffBaseHz = 800.0f;
    float resonance = 0.15f;
    float drive = 1.2f;
    float ampA = 0.18f, ampD = 1.2f, ampS = 0.8f, ampR = 2.5f;
    float filA = 0.30f, filD = 2.0f, filS = 0.5f, filR = 3.0f;
    float filEnvAmt = 0.9f;
    float keyTrack = 0.35f;
    float pwm = 0.40f;
    float pwmLfoAmt = 0.05f;
    float unisonDetuneCents = 12.0f;
    float stereoSpread = 0.9f;
    float microDelayMsMax = 6.0f;
    float lfo1Hz = 0.08f, lfo1Amt = 0.03f, lfo2Hz = 0.15f, lfo2Amt = 0.05f;
    float lfoPhase1 = 0.0f, lfoPhase2 = 0.0f;

    PolyBlepOsc oscSaw1[kUnison], oscSaw2[kUnison], oscPulse[kUnison], subTri[kUnison];
    Drift drift1[kUnison], drift2[kUnison], driftP[kUnison];
    float unisonPan[kUnison] = {};
    float unisonDelaySamples[kUnison] = {};
    juce::dsp::LadderFilter<float> ladder;
    juce::AudioBuffer<float> ladderBuffer { 2, 1 };
    MicroDelayStereo microDelay;
    juce::ADSR ampAdsr, filtAdsr;
};

//==============================================================================
class SuperWidePadSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
PadSynthesizer::PadSynthesizer()
{
    synth.clearVoices();
    for (int i = 0; i < 12; ++i)
        synth.addVoice (new SuperWidePadVoice());
    synth.clearSounds();
    synth.addSound (new SuperWidePadSound());
}

PadSynthesizer::~PadSynthesizer() {}

void PadSynthesizer::prepare (double sampleRate, int samplesPerBlock)
{
    fxSpec.sampleRate = sampleRate;
    fxSpec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    fxSpec.numChannels = 2;

    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SuperWidePadVoice*> (synth.getVoice (i)))
            v->prepare (sampleRate, samplesPerBlock);

    chorus.reset();
    chorus.prepare (fxSpec);
    reverb.reset();
    reverb.prepare (fxSpec);
    revHp.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 200.0f);
    revLp.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 9000.0f);
    revHp.prepare (fxSpec);
    revLp.prepare (fxSpec);
    fxBuffer.setSize (2, samplesPerBlock);
}

void PadSynthesizer::reset()
{
    synth.allNotesOff (0, true);
    chorus.reset();
    reverb.reset();
    revHp.reset();
    revLp.reset();
}

void PadSynthesizer::allNotesOff()
{
    synth.allNotesOff (0, true);
}

void PadSynthesizer::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                      juce::MidiBuffer& midiBuffer,
                                      int startSample,
                                      int numSamples,
                                      juce::AudioProcessorValueTreeState& apvts)
{
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SuperWidePadVoice*> (synth.getVoice (i)))
            v->setParams (apvts);

    synth.renderNextBlock (outputBuffer, midiBuffer, startSample, numSamples);

    if (outputBuffer.getNumChannels() < 2 || numSamples <= 0) return;

    fxBuffer.setSize (2, numSamples, false, false, true);
    for (int ch = 0; ch < 2; ++ch)
        fxBuffer.copyFrom (ch, 0, outputBuffer, ch, startSample, numSamples);

    juce::dsp::AudioBlock<float> workBlock (fxBuffer);
    juce::dsp::ProcessContextNonReplacing<float> workCtx (workBlock, workBlock);

    chorus.setRate (apvts.getRawParameterValue ("PadChorusRate")->load());
    chorus.setDepth (apvts.getRawParameterValue ("PadChorusDepth")->load());
    chorus.setCentreDelay (apvts.getRawParameterValue ("PadChorusDelay")->load());
    chorus.setMix (apvts.getRawParameterValue ("PadChorusMix")->load());
    chorus.process (workCtx);

    double sr = apvts.processor.getSampleRate();
    revHp.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, apvts.getRawParameterValue ("PadRevLowCut")->load());
    revLp.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, apvts.getRawParameterValue ("PadRevHighCut")->load());
    revHp.process (workCtx);
    revLp.process (workCtx);

    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = apvts.getRawParameterValue ("PadRevRoom")->load();
    rp.damping = apvts.getRawParameterValue ("PadRevDamp")->load();
    rp.wetLevel = apvts.getRawParameterValue ("PadRevWet")->load();
    rp.dryLevel = 1.0f - rp.wetLevel;
    rp.width = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
    reverb.process (workCtx);

    for (int ch = 0; ch < 2; ++ch)
        outputBuffer.copyFrom (ch, startSample, fxBuffer, ch, 0, numSamples);

    float outGain = apvts.getRawParameterValue ("PadOut")->load();
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
    {
        auto* data = outputBuffer.getWritePointer (ch);
        for (int i = startSample; i < startSample + numSamples; ++i)
            data[i] = softClip (data[i] * outGain);
    }
}
