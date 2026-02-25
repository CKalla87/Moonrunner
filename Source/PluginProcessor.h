/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "FMSynthesizer.h"
#include "AnalogSynthesizer.h"
#include "PadSynthesizer.h"
#include "SamplerEngine.h"

//==============================================================================
/**
*/
class MoonrunnerAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    MoonrunnerAudioProcessor();
    ~MoonrunnerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // MIDI message injection from UI (for keyboard component)
    void processMidiMessage (const juce::MidiMessage& message);

    //==============================================================================
    // Parameter management
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    // Use pointer to defer initialization until constructor body (avoids hangs during member init)
    std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;

    // Synthesis mode parameter
    std::atomic<float>* synthesisModeParam = nullptr; // 0=FM, 1=Analog, 2=Sampler
    
    // Master parameters
    std::atomic<float>* masterVolumeParam = nullptr;
    std::atomic<float>* masterTuneParam = nullptr;
    
    // Synth parameters (mainly for Analog mode)
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* filterCutoffParam = nullptr;
    std::atomic<float>* filterResonanceParam = nullptr;
    std::atomic<float>* lfoRateParam = nullptr;
    std::atomic<float>* lfoAmountParam = nullptr;
    std::atomic<float>* lfoWaveformParam = nullptr;
    std::atomic<float>* lfoDestinationParam = nullptr;
    std::atomic<float>* oscWaveformParam = nullptr;

    // Moonrunner v2 UI params
    std::atomic<float>* oscTypeParam = nullptr;
    std::atomic<float>* attack2Param = nullptr;
    std::atomic<float>* decay2Param = nullptr;
    std::atomic<float>* sustain2Param = nullptr;
    std::atomic<float>* release2Param = nullptr;
    std::atomic<float>* cutoffParam = nullptr;
    std::atomic<float>* resonanceParam = nullptr;

    // Active note count for UI (IDLE/ACTIVE pill, meter animation)
    std::atomic<int> activeNoteCount { 0 };

private:
    //==============================================================================
    FMSynthesizer fmSynth;
    AnalogSynthesizer analogSynth;
    PadSynthesizer padSynth;
    SamplerEngine samplerEngine;

    int currentSynthesisMode = 0; // 0=FM, 1=Analog, 2=Sampler, 3=Pad
    
    double currentSampleRate = 44100.0;
    
    // Thread-safe MIDI message queue for UI keyboard
    juce::CriticalSection midiMessageLock;
    juce::Array<juce::MidiMessage> pendingMidiMessages;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoonrunnerAudioProcessor)
};

