/*
  ==============================================================================

    PluginProcessor.h
    Created: 5 Sep 2023
    Author:  binya

  ==============================================================================

    Welcome to the Just a Sample source code! This is my first plugin and I am 
    excited to share it as a free, open-source project. I hope this can serve
    as a learning tool for others and a useful tool for musicians.

    Even a simple plugin like this one requires some architecture considerations:
    -   PluginProcessor is self-contained and exposes an API for the PluginEditor.
        Ut also reacts to parameter changes which can be triggered by anything 
        (the host DAW or the Editor). 
    -   In order to reduce complexity, I chose to not make individual components
        completely modular. This means they individually respond to APVTS changes.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "RubberBandStretcher.h"
#include "CustomLookAndFeel.h"
#include "sampler/CustomSamplerVoice.h"
#include "sampler/CustomSamplerSound.h"
#include "utilities/PitchDetector.h"
#include "Utilities/DeviceRecorder.h"

class JustaSampleAudioProcessor : public AudioProcessor, public ValueTree::Listener, public AudioProcessorValueTreeState::Listener,
    public Thread::Listener, public DeviceRecorderListener
#if JucePlugin_Enable_ARA
    , public juce::AudioProcessorARAExtension
#endif
{
public:
    JustaSampleAudioProcessor();
    ~JustaSampleAudioProcessor() override;

    /** Returns whether the sample buffer is too large to be stored in the plugin data */
    bool sampleBufferNeedsReference() const;

    /** Whether the processor can handle a filePath's extension */
    bool canLoadFileExtension(const String& filePath);

    //==============================================================================
    /** Open a file chooser */
    void openFileChooser(const String& message, int flags, std::function<void(const FileChooser&)> callback, bool wavOnly = false);

    /** Detects the pitch of the sample between the supplied bounds and adjusts the tuning parameters accordingly */
    bool startPitchDetectionRoutine(int startSample, int endSample);

    /** Stop all the voices from playing */
    void haltVoices();

    //==============================================================================
    const juce::AudioBuffer<float>& getSampleBuffer() const { return sampleBuffer; }
    double getBufferSampleRate() const { return bufferSampleRate; }
    const juce::Array<CustomSamplerVoice*>& getSamplerVoices() const { return samplerVoices; }

    /** The APVTS is the central object storing plugin state and audio processing parameters. See PluginParameters.h. */
    juce::AudioProcessorValueTreeState& APVTS() { return apvts; }
    juce::UndoManager& getUndoManager() { return undoManager; }
    DeviceRecorder& getRecorder() { return deviceRecorder; }
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }
    juce::String getWildcardFilter() const { return formatManager.getWildcardForAllFormats(); }

    //==============================================================================
    const juce::var& p(juce::Identifier identifier) const { return apvts.getParameterAsValue(identifier).getValue(); }
    juce::Value pv(juce::Identifier identifier) const { return apvts.getParameterAsValue(identifier); }
    const juce::var& sp(juce::Identifier identifier) const { return apvts.state.getProperty(identifier); }
    juce::Value spv(juce::Identifier identifier) { return apvts.state.getPropertyAsValue(identifier, apvts.undoManager); }

private:
    //==============================================================================
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif
    const juce::String getName() const override { return JucePlugin_Name; };
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override { return 0.0; };
    int getNumPrograms() override { return 1; };
    int getCurrentProgram() override { return 0; };
    void setCurrentProgram(int) override {};
    const juce::String getProgramName(int) override { return {}; };
    void changeProgramName(int, const juce::String&) override {};
    bool hasEditor() const override { return true; };
    juce::AudioProcessorEditor* createEditor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    /** The plugin's state information includes the full APVTS (with non-parameter values) and audio data if a file 
        reference is not being used.
    */
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    /** Loads an audio buffer into the synth, preparing everything for playback.
        Note that this method takes ownership of the buffer.
     */
    bool loadSample(juce::AudioBuffer<float>& sample, int sampleRate);


    /** Load a file/sample and reset to default parameters, intended for when a new sample is loaded not from preset
        reloadSample reloads whatever is in the sample buffer, tries is used for repeated prompting if necessary
    */
    bool loadSampleAndReset(const String& path, bool reloadSample = false, int tries = 0);

    /** Load a file, updates the sampler, returns whether the file was loaded successfully */
    bool loadFile(const juce::String& path, const juce::String& expectedHash = "");

    /** Updates the sampler with a new AudioBuffer */
    void updateSamplerSound(AudioBuffer<float>& sample);



    /** Set the plugin's latency according to processing parameters (necessary for preprocessing options) */
    void setProperLatency();

    /** This signature is necessary for use in the APVTS callback */
    void setProperLatency(PluginParameters::PLAYBACK_MODES mode);

    /** Reset the sampler voices */
    void resetSamplerVoices();

    /** Uses MD-5 hashing to generate an identifier for the AudioBuffer */
    juce::String getSampleHash(const juce::AudioBuffer<float>& buffer) const;

    /** This is where the plugin should react to processing related property changes */
    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;
    void parameterChanged(const String& parameterID, float newValue) override;

    // Bounds checking for the loop start and end portions 
    void updateLoopStartPortionBounds();
    void updateLoopEndPortionBounds();
    int visibleSamples() const;

    //==============================================================================
    void recordingStarted() override {};
    void recordingFinished(AudioBuffer<float> recording, int recordingSampleRate) override;

    /** This runs when the pitch detection thread finishes. */
    void exitSignalSent() override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    juce::UndoManager undoManager;
    AudioFormatManager formatManager;
    AudioDeviceManager deviceManager;

    AudioBuffer<float> sampleBuffer;
    double bufferSampleRate{ 0. };
    Array<CustomSamplerVoice*> samplerVoices;

    std::unique_ptr<FileChooser> fileChooser;

    //==============================================================================
    Synthesiser synth;

    WildcardFileFilter fileFilter;
    std::unique_ptr<AudioFormatReader> formatReader;

    DeviceRecorder deviceRecorder;

    PitchDetector pitchDetector;

    CustomLookAndFeel lookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JustaSampleAudioProcessor)
};