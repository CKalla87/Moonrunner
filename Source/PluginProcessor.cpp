/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <ctime>
#include <cstdlib>
#include <cstdio>

//==============================================================================
// File-based logging function (works even when stderr is redirected)
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
    // Also try stderr
    fprintf(stderr, "Moonrunner: %s\n", message);
    fflush(stderr);
}

MoonrunnerAudioProcessor::MoonrunnerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    logToFile("Processor constructor start");
    DBG("Moonrunner: Processor constructor start");
    
    // Initialize APVTS in constructor body (not member init) to avoid hangs
    logToFile("Initializing APVTS...");
    apvts = std::make_unique<juce::AudioProcessorValueTreeState>(
        *this, nullptr, "Parameters", createParameterLayout());
    logToFile("APVTS initialized");
    
           // Get parameter pointers - add defensive checks
           synthesisModeParam = apvts->getRawParameterValue("SYNTHESIS_MODE");
           masterVolumeParam = apvts->getRawParameterValue("MASTER_VOLUME");
           masterTuneParam = apvts->getRawParameterValue("MASTER_TUNE");
           
           // Synth parameters
           attackParam = apvts->getRawParameterValue("ATTACK");
           decayParam = apvts->getRawParameterValue("DECAY");
           sustainParam = apvts->getRawParameterValue("SUSTAIN");
           releaseParam = apvts->getRawParameterValue("RELEASE");
           filterCutoffParam = apvts->getRawParameterValue("FILTER_CUTOFF");
           filterResonanceParam = apvts->getRawParameterValue("FILTER_RESONANCE");
           lfoRateParam = apvts->getRawParameterValue("LFO_RATE");
           lfoAmountParam = apvts->getRawParameterValue("LFO_AMOUNT");
           lfoWaveformParam = apvts->getRawParameterValue("LFO_WAVEFORM");
           lfoDestinationParam = apvts->getRawParameterValue("LFO_DESTINATION");
           oscWaveformParam = apvts->getRawParameterValue("OSC_WAVEFORM");

           // Moonrunner v2 UI params
           oscTypeParam = apvts->getRawParameterValue("oscType");
           attack2Param = apvts->getRawParameterValue("attack");
           decay2Param = apvts->getRawParameterValue("decay");
           sustain2Param = apvts->getRawParameterValue("sustain");
           release2Param = apvts->getRawParameterValue("release");
           cutoffParam = apvts->getRawParameterValue("cutoff");
           resonanceParam = apvts->getRawParameterValue("resonance");
           
           logToFile("Parameters retrieved");
           DBG("Moonrunner: Parameters retrieved");
    
    logToFile("Processor constructor complete");
    DBG("Moonrunner: Processor constructor complete");
}

MoonrunnerAudioProcessor::~MoonrunnerAudioProcessor()
{
}

//==============================================================================
const juce::String MoonrunnerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MoonrunnerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MoonrunnerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MoonrunnerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MoonrunnerAudioProcessor::getTailLengthSeconds() const
{
    return 2.0; // Allow for release tails
}

int MoonrunnerAudioProcessor::getNumPrograms()
{
    return 1;
}

int MoonrunnerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MoonrunnerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MoonrunnerAudioProcessor::getProgramName (int index)
{
    return {};
}

void MoonrunnerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MoonrunnerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    fmSynth.prepare (sampleRate, samplesPerBlock);
    analogSynth.prepare (sampleRate, samplesPerBlock);
    samplerEngine.prepare (sampleRate, samplesPerBlock);
}

void MoonrunnerAudioProcessor::releaseResources()
{
    fmSynth.reset();
    analogSynth.reset();
    samplerEngine.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MoonrunnerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void MoonrunnerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Clear the buffer for synth output
    buffer.clear();

    // Get parameter values (with null checks for safety)
    float synthesisMode = (synthesisModeParam != nullptr) ? synthesisModeParam->load() : 0.0f;
    float masterVolume = (masterVolumeParam != nullptr) ? masterVolumeParam->load() : 0.7f;
    float masterTune = (masterTuneParam != nullptr) ? masterTuneParam->load() : 0.0f;
    
    int newMode = juce::jlimit (0, 2, static_cast<int> (synthesisMode)); // 0=FM, 1=Analog, 2=Sampler
    if (newMode != currentSynthesisMode)
    {
        // Switch synthesis mode - turn off all notes
        fmSynth.allNotesOff();
        analogSynth.allNotesOff();
        samplerEngine.allNotesOff();
        currentSynthesisMode = newMode;
    }

    // Collect UI-generated MIDI messages first
    juce::Array<juce::MidiMessage> uiMidiMessages;
    {
        const juce::ScopedLock sl (midiMessageLock);
        uiMidiMessages = pendingMidiMessages;
        pendingMidiMessages.clear();
    }
    
    // Add UI MIDI messages to the buffer
    for (const auto& message : uiMidiMessages)
    {
        midiMessages.addEvent (message, 0);
    }
    
    // Process MIDI messages
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        
        // Check if this message came from the UI (to avoid double-updating keyboardState)
        bool isFromUI = false;
        for (const auto& uiMsg : uiMidiMessages)
        {
            if (uiMsg.getNoteNumber() == message.getNoteNumber() && 
                uiMsg.isNoteOn() == message.isNoteOn() && 
                uiMsg.isNoteOff() == message.isNoteOff())
            {
                isFromUI = true;
                break;
            }
        }
        
        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            float velocity = message.getVelocity() / 127.0f;
            
            // Only update on-screen keyboard state for external MIDI (not UI-generated)
            if (!isFromUI)
            {
                if (auto* editor = dynamic_cast<MoonrunnerAudioProcessorEditor*> (getActiveEditor()))
                {
                    juce::Component::SafePointer<MoonrunnerAudioProcessorEditor> safeEditor (editor);
                    juce::MessageManager::callAsync ([safeEditor, noteNumber, velocity]()
                    {
                        if (safeEditor != nullptr)
                            safeEditor->updateKeyboardState (noteNumber, true, velocity);
                    });
                }
            }
            
            if (currentSynthesisMode == 0) // FM
                fmSynth.noteOn (noteNumber, velocity);
            else if (currentSynthesisMode == 1) // Analog
                analogSynth.noteOn (noteNumber, velocity);
            else if (currentSynthesisMode == 2) // Sampler
                samplerEngine.noteOn (noteNumber, velocity);
        }
        else if (message.isNoteOff())
        {
            int noteNumber = message.getNoteNumber();
            
            // Only update on-screen keyboard state for external MIDI (not UI-generated)
            if (!isFromUI)
            {
                if (auto* editor = dynamic_cast<MoonrunnerAudioProcessorEditor*> (getActiveEditor()))
                {
                    juce::Component::SafePointer<MoonrunnerAudioProcessorEditor> safeEditor (editor);
                    juce::MessageManager::callAsync ([safeEditor, noteNumber]()
                    {
                        if (safeEditor != nullptr)
                            safeEditor->updateKeyboardState (noteNumber, false, 0.0f);
                    });
                }
            }
            
            if (currentSynthesisMode == 0) // FM
                fmSynth.noteOff (noteNumber);
            else if (currentSynthesisMode == 1) // Analog
                analogSynth.noteOff (noteNumber);
            else if (currentSynthesisMode == 2) // Sampler
                samplerEngine.noteOff (noteNumber);
        }
        else if (message.isPitchWheel())
        {
            float pitchBend = (message.getPitchWheelValue() - 8192) / 8192.0f * 2.0f; // ±2 semitones
            pitchBend += masterTune;
            
            if (currentSynthesisMode == 0) // FM
                fmSynth.setPitchBend (pitchBend);
            else if (currentSynthesisMode == 1) // Analog
                analogSynth.setPitchBend (pitchBend);
            else if (currentSynthesisMode == 2) // Sampler
                samplerEngine.setPitchBend (pitchBend);
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            // Update on-screen keyboard state for external MIDI
            if (!isFromUI)
            {
                if (auto* editor = dynamic_cast<MoonrunnerAudioProcessorEditor*> (getActiveEditor()))
                {
                    juce::Component::SafePointer<MoonrunnerAudioProcessorEditor> safeEditor (editor);
                    juce::MessageManager::callAsync ([safeEditor]()
                    {
                        if (safeEditor != nullptr)
                            safeEditor->clearKeyboardState();
                    });
                }
            }
            
            fmSynth.allNotesOff();
            analogSynth.allNotesOff();
            samplerEngine.allNotesOff();
        }
    }

    // Apply synth parameters (use v2 UI params: attack, decay, sustain, release, cutoff, resonance)
    if (currentSynthesisMode == 0) // FM
    {
        // ADSR Envelope - use v2 params so UI sliders work
        auto atk = (attack2Param != nullptr) ? attack2Param->load() : (attackParam ? attackParam->load() : 0.1f);
        auto dcy = (decay2Param != nullptr) ? decay2Param->load() : (decayParam ? decayParam->load() : 0.3f);
        auto sus = (sustain2Param != nullptr) ? sustain2Param->load() : (sustainParam ? sustainParam->load() : 0.7f);
        auto rel = (release2Param != nullptr) ? release2Param->load() : (releaseParam ? releaseParam->load() : 0.5f);
        fmSynth.setEnvelopeAttack (atk);
        fmSynth.setEnvelopeDecay (dcy);
        fmSynth.setEnvelopeSustain (sus);
        fmSynth.setEnvelopeRelease (rel);
        
        // Filter - use v2 params (resonance 0.1-20 -> 0-0.9)
        auto cut = (cutoffParam != nullptr) ? cutoffParam->load() : (filterCutoffParam ? filterCutoffParam->load() : 2000.0f);
        auto res = (resonanceParam != nullptr) ? juce::jlimit (0.0f, 0.9f, (resonanceParam->load() - 0.1f) / 19.9f)
                                               : (filterResonanceParam ? filterResonanceParam->load() : 0.5f);
        fmSynth.setFilterCutoff (cut);
        fmSynth.setFilterResonance (res);
    }
    else if (currentSynthesisMode == 1) // Analog
    {
        // ADSR Envelope (prefer v2 params)
        auto atk = (attack2Param != nullptr) ? attack2Param->load() : (attackParam ? attackParam->load() : 0.1f);
        auto dcy = (decay2Param != nullptr) ? decay2Param->load() : (decayParam ? decayParam->load() : 0.3f);
        auto sus = (sustain2Param != nullptr) ? sustain2Param->load() : (sustainParam ? sustainParam->load() : 0.7f);
        auto rel = (release2Param != nullptr) ? release2Param->load() : (releaseParam ? releaseParam->load() : 0.5f);
        analogSynth.setEnvelopeAttack (atk);
        analogSynth.setEnvelopeDecay (dcy);
        analogSynth.setEnvelopeSustain (sus);
        analogSynth.setEnvelopeRelease (rel);

        // Filter (prefer v2 params; map resonance 0.1-20 to 0-0.9)
        auto cut = (cutoffParam != nullptr) ? cutoffParam->load() : (filterCutoffParam ? filterCutoffParam->load() : 2000.0f);
        auto res = (resonanceParam != nullptr) ? juce::jlimit (0.0f, 0.9f, (resonanceParam->load() - 0.1f) / 19.9f)
                                               : (filterResonanceParam ? filterResonanceParam->load() : 0.5f);
        analogSynth.setFilterCutoff (cut);
        analogSynth.setFilterResonance (res);
        
        // LFO
        if (lfoRateParam != nullptr)
            analogSynth.setLFORate (lfoRateParam->load());
        if (lfoAmountParam != nullptr)
            analogSynth.setLFOAmount (lfoAmountParam->load());
        if (lfoWaveformParam != nullptr)
            analogSynth.setLFOWaveform (static_cast<int>(lfoWaveformParam->load()));
        if (lfoDestinationParam != nullptr)
            analogSynth.setLFODestination (static_cast<int>(lfoDestinationParam->load()));
        
        // Oscillator (oscType: 0=sine, 1=square, 2=sawtooth, 3=triangle -> Analog: 4=sine, 1=square, 0=saw, 3=triangle)
        int oscIdx = (oscTypeParam != nullptr) ? juce::roundToInt (oscTypeParam->load() * 3.0f) : 2;
        int waveform = (oscIdx == 0) ? 4 : (oscIdx == 1) ? 1 : (oscIdx == 2) ? 0 : 3;  // sine=4 (add to Analog)
        analogSynth.setOscillatorWaveform (0, waveform);
        analogSynth.setOscillatorWaveform (1, waveform);
    }
    else if (currentSynthesisMode == 2) // Sampler
    {
        // Attack/Release - use v2 params so UI sliders work
        auto atk = (attack2Param != nullptr) ? attack2Param->load() : (attackParam ? attackParam->load() : 0.1f);
        auto rel = (release2Param != nullptr) ? release2Param->load() : (releaseParam ? releaseParam->load() : 0.5f);
        samplerEngine.setAttack (atk);
        samplerEngine.setRelease (rel);
        
        // Filter - use v2 params (resonance 0.1-20 -> 0-0.9)
        auto cut = (cutoffParam != nullptr) ? cutoffParam->load() : (filterCutoffParam ? filterCutoffParam->load() : 2000.0f);
        auto res = (resonanceParam != nullptr) ? juce::jlimit (0.0f, 0.9f, (resonanceParam->load() - 0.1f) / 19.9f)
                                               : (filterResonanceParam ? filterResonanceParam->load() : 0.5f);
        samplerEngine.setFilterCutoff (cut);
        samplerEngine.setFilterResonance (res);
    }

    // Render audio from the active synthesis engine
    if (currentSynthesisMode == 0) // FM
    {
        fmSynth.renderNextBlock (buffer, 0, buffer.getNumSamples());
    }
    else if (currentSynthesisMode == 1) // Analog
    {
        analogSynth.renderNextBlock (buffer, 0, buffer.getNumSamples());
    }
    else if (currentSynthesisMode == 2) // Sampler
    {
        samplerEngine.renderNextBlock (buffer, 0, buffer.getNumSamples());
    }

    // Apply master volume
    buffer.applyGain (masterVolume);
}

//==============================================================================
bool MoonrunnerAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* MoonrunnerAudioProcessor::createEditor()
{
    logToFile("createEditor() called");
    fprintf(stderr, "Moonrunner: createEditor() called\n");
    fflush(stderr);
    DBG("Moonrunner: createEditor() called");
    
    logToFile("About to create editor instance");
    fprintf(stderr, "Moonrunner: About to create editor instance\n");
    fflush(stderr);
    
    auto* editor = new MoonrunnerAudioProcessorEditor (*this);
    
    logToFile("Editor instance created successfully");
    fprintf(stderr, "Moonrunner: Editor instance created successfully\n");
    fflush(stderr);
    return editor;
}

//==============================================================================
void MoonrunnerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (apvts != nullptr)
    {
        auto state = apvts->copyState();
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }
}

void MoonrunnerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (apvts != nullptr)
    {
        std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

        if (xmlState.get() != nullptr)
            if (xmlState->hasTagName (apvts->state.getType()))
                apvts->replaceState (juce::ValueTree::fromXml (*xmlState));
    }
}

//==============================================================================
void MoonrunnerAudioProcessor::processMidiMessage (const juce::MidiMessage& message)
{
    // Queue MIDI message to be processed in audio thread (thread-safe)
    const juce::ScopedLock sl (midiMessageLock);
    pendingMidiMessages.add (message);
}

//==============================================================================
// Parameter layout creation
juce::AudioProcessorValueTreeState::ParameterLayout MoonrunnerAudioProcessor::createParameterLayout()
{
    // Note: logToFile is defined above, but we can't call it from a static function
    // that might be called during static initialization. Use stderr only here.
    fprintf(stderr, "Moonrunner: createParameterLayout() called\n");
    fflush(stderr);
    DBG("Moonrunner: createParameterLayout() called");
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    fprintf(stderr, "Moonrunner: Creating SYNTHESIS_MODE parameter\n");
    fflush(stderr);
    DBG("Moonrunner: Creating SYNTHESIS_MODE parameter");
    // Synthesis Mode: 0=FM, 1=Analog, 2=Sampler (default Sampler)
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID ("SYNTHESIS_MODE", 1), "Synthesis Mode",
        juce::StringArray ("FM", "Analog", "Sampler"),
        2
    ));

    fprintf(stderr, "Moonrunner: Creating MASTER_VOLUME parameter\n");
    fflush(stderr);
    DBG("Moonrunner: Creating MASTER_VOLUME parameter");
    // Master Volume: 0 to 100%
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("MASTER_VOLUME", 1), "Master Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.7f, ""
    ));

    fprintf(stderr, "Moonrunner: Creating MASTER_TUNE parameter\n");
    fflush(stderr);
    DBG("Moonrunner: Creating MASTER_TUNE parameter");
    // Master Tune: -12 to +12 semitones
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("MASTER_TUNE", 1), "Master Tune",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f),
        0.0f, "semitones"
    ));

    // ADSR Envelope (for Analog mode) - Jump brass
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("ATTACK", 1), "Attack",
        juce::NormalisableRange<float> (0.0f, 5.0f, 0.01f),
        0.005f, "s"
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("DECAY", 1), "Decay",
        juce::NormalisableRange<float> (0.0f, 5.0f, 0.01f),
        0.2f, "s"
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("SUSTAIN", 1), "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.75f, ""
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("RELEASE", 1), "Release",
        juce::NormalisableRange<float> (0.0f, 5.0f, 0.01f),
        0.3f, "s"
    ));

    // Filter (for Analog mode) - Jump / OB-Xa brass
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("FILTER_CUTOFF", 1), "Filter Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f),
        2800.0f, "Hz"
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("FILTER_RESONANCE", 1), "Filter Resonance",
        juce::NormalisableRange<float> (0.0f, 0.9f, 0.01f), // Limit to 0.9 to prevent instability
        0.35f, ""
    ));

    // LFO (for Analog mode) - minimal for Jump (static brass)
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("LFO_RATE", 1), "LFO Rate",
        juce::NormalisableRange<float> (0.1f, 20.0f, 0.1f),
        0.35f, "Hz"
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("LFO_AMOUNT", 1), "LFO Amount",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.05f, ""
    ));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID ("LFO_WAVEFORM", 1), "LFO Waveform",
        juce::StringArray ("Sine", "Triangle", "Square", "Saw"),
        0
    ));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID ("LFO_DESTINATION", 1), "LFO Destination",
        juce::StringArray ("Filter", "Pitch", "Both"),
        0
    ));

    // Oscillator Waveform (for Analog mode)
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID ("OSC_WAVEFORM", 1), "Oscillator Waveform",
        juce::StringArray ("Saw", "Square", "Pulse", "Triangle"),
        0
    ));

    // New Moonrunner v2 UI parameters - Van Halen Jump / Oberheim OB-Xa brass defaults
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID ("oscType", 2), "Oscillator Type",
        juce::StringArray ("sine", "square", "sawtooth", "triangle"),
        2  // sawtooth - Jump brass
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("attack", 2), "Attack",
        juce::NormalisableRange<float> (0.01f, 2.0f, 0.01f),
        0.005f, "s"  // Punchy brass attack
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("decay", 2), "Decay",
        juce::NormalisableRange<float> (0.01f, 2.0f, 0.01f),
        0.2f, "s"
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("sustain", 2), "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.75f, ""
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("release", 2), "Release",
        juce::NormalisableRange<float> (0.01f, 3.0f, 0.01f),
        0.3f, "s"
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("cutoff", 2), "Cutoff",
        juce::NormalisableRange<float> (100.0f, 8000.0f, 10.0f),
        2800.0f, "Hz"  // Open filter for Jump bite
    ));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("resonance", 2), "Resonance",
        juce::NormalisableRange<float> (0.1f, 20.0f, 0.1f),
        7.0f, ""  // ~0.35 - moderate resonance for brass
    ));

    fprintf(stderr, "Moonrunner: createParameterLayout() complete, returning layout\n");
    fflush(stderr);
    DBG("Moonrunner: createParameterLayout() complete, returning layout");
    return { params.begin(), params.end() };
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    logToFile("createPluginFilter() called - ENTRY POINT");
    DBG("Moonrunner: createPluginFilter() called - ENTRY POINT");
    
    logToFile("About to create MoonrunnerAudioProcessor instance");
    DBG("Moonrunner: About to create MoonrunnerAudioProcessor instance");
    
    auto* processor = new MoonrunnerAudioProcessor();
    
    logToFile("MoonrunnerAudioProcessor instance created successfully");
    DBG("Moonrunner: MoonrunnerAudioProcessor instance created successfully");
    
    return processor;
}

