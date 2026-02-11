/*
  ==============================================================================

    PluginProcessor.h
    Created: 5 Sep 2023
    Author:  binya

  ==============================================================================

    Welcome to the Just a Sample source code! This is my first plugin, and I am 
    excited to share it as a free, open-source project. I hope this can serve
    as a learning tool for other programmers and a useful plugin for musicians.

    Even a "simple" plugin like this one requires some architecture considerations.
    Care was taken to make PluginProcessor self-contained with a clear API exposed
    to the PluginEditor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "CustomLookAndFeel.h"
#include "Sampler/CustomSamplerVoice.h"
#include "Sampler/CustomSynthesizer.h"
#include "Utilities/PitchDetector.h"
#include "Utilities/DeviceRecorder.h"
#include "Utilities/Reaper/ReaperVST3Extensions.h"
#include "Utilities/SampleLoader.h"
#include "Utilities/MTS/libMTSClient.h"

class JustaSampleAudioProcessor final : public juce::AudioProcessor, public juce::Thread::Listener, public DeviceRecorderListener
#if JucePlugin_Enable_ARA
    , public juce::AudioProcessorARAExtension
#endif
{
public:
    JustaSampleAudioProcessor();
    ~JustaSampleAudioProcessor() override;

    /** Returns whether the sample buffer is too large to be stored in the plugin data */
    bool sampleBufferNeedsReference() const { return sampleBufferNeedsReference(sampleBuffer); }
    bool sampleBufferNeedsReference(const juce::AudioBuffer<float>& buffer) const;

    /** Whether the processor can handle a filePath's extension */
    bool canLoadFileExtension(const juce::String& filePath) const;

    //==============================================================================
    /** Asynchronously loads an audio buffer from a file path, returning (in a callback) whether the sample
        was loaded successfully. If an expected hash is provided and continueWithWrongHash = false, then the
        sample will only be loaded if the hash matches. If continueWithWrongHash = true, then the sample will
        be loaded and parameters will be reset.
    */
    void loadSampleFromPath(const juce::String& path, bool resetParameters = true, const juce::String& expectedHash = "", bool continueWithWrongHash = false,
        const std::function<void(bool loadedSuccessfully)>& callback = [](bool) -> void {});

    /** Open a file chooser */
    void openFileChooser(const juce::String& message, int flags, const std::function<void(const juce::FileChooser&)>& callback, bool wavOnly = false);

    /** Detects the pitch of the sample between the supplied bounds and adjusts the tuning parameters accordingly */
    bool startPitchDetectionRoutine(int startSample, int endSample);

    /** Stop all the voices from playing */
    void haltVoices();

    /** Plays an A5 note */
    void playVoice();

    //==============================================================================
    const juce::AudioBuffer<float>& getSampleBuffer() const { return sampleBuffer; }
    float getBufferSampleRate() const { return bufferSampleRate; }
    const juce::OwnedArray<CustomSamplerVoice>& getSamplerVoices() const { return samplerVoices; }

    /** The APVTS is the central object storing plugin state and audio processing parameters. See PluginParameters.h. */
    juce::AudioProcessorValueTreeState& APVTS() { return apvts; }
    juce::UndoManager& getUndoManager() { return undoManager; }
    /** The plugin state object stores non-parameter data, to be used in a thread-safe way */
    PluginParameters::State& getPluginState() { return pluginState; }

    DeviceRecorder& getRecorder() { return deviceRecorder; }
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }
    /** We initialize the device manager lazily from the UI thread, so that we only block the plugin when necessary. Call this
        from the Editor when you need the device manager configured appropriately.
    */
    void initializeDeviceManager();

    juce::String getWildcardFilter() const { return formatManager.getWildcardForAllFormats(); }
    const SampleLoader& getSampleLoader() const { return sampleLoader; }

    /** Use to determine whether the editor should reset UI parameters */
    bool hasLoadedFromReaper() const { return loadedFromReaper.load(); }
    void setLoadedFromReaper(const bool loaded) { loadedFromReaper.store(loaded); }

    //==============================================================================
    juce::var p(const juce::Identifier& identifier) const { return apvts.getParameterAsValue(identifier).getValue(); }
    juce::Value pv(const juce::Identifier& identifier) const { return apvts.getParameterAsValue(identifier); }

private:
    const juce::var& sp(const juce::Identifier& identifier) const { return apvts.state.getProperty(identifier); }
    juce::Value spv(const juce::Identifier& identifier) { return apvts.state.getPropertyAsValue(identifier, apvts.undoManager); }

    //==============================================================================
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout& layouts) const override;
#endif
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    juce::VST3ClientExtensions* getVST3ClientExtensions() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    /** Add or subtract voices if necessary */
    void adjustVoiceCount(int count = -1);

    //==============================================================================
    /** The plugin's state information includes the full APVTS (with non-parameter values) and audio data if a file 
        reference is not being used.
    */
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    /** Loads an audio buffer into the synth, preparing everything for playback. If 
        resetParameters is true, the plugin's parameters are reset to default, matching
        the new sample. Otherwise, the assumption is that the parameters are in a valid state.
        Note that this method uses std::move on the sample.
        Also, if necessary, set pluginState.filePath before calling this method, so that the editor syncs correctly.
     */
    void loadSample(juce::AudioBuffer<float>& sample, int sampleRate, bool resetParameters = true, const juce::String& precomputedHash = "");

    //==============================================================================
    void recordingStarted() override {}
    void recordingFinished(juce::AudioBuffer<float> recordingBuffer, int recordingSampleRate) override;

    /** This runs when the pitch detection thread finishes. */
    void exitSignalSent() override;

    //==============================================================================
    int visibleSamples() const;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    juce::UndoManager undoManager;
    PluginParameters::State pluginState;

    CustomSynthesizer synth;

    /** Note that this is referenced directly by the Editor. As such, it should only be modified in the Message Thread. */
    juce::AudioBuffer<float> sampleBuffer;
    float bufferSampleRate{ 0.f };
    SamplerParameters samplerSound;
    /** We manage MAX_VOICES for the duration of the plugin and control how many the synth has access to */
    juce::OwnedArray<CustomSamplerVoice> samplerVoices;
    juce::CriticalSection voiceLock;

    juce::PluginHostType hostType;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::AudioFormatManager formatManager;
    juce::WildcardFileFilter fileFilter;

    SampleLoader sampleLoader;
    juce::String lastLoadAttempt;
    /** We use this to notify the editor that the sample was loaded from Reaper, since this should be treated as a user load */
    std::atomic<bool> loadedFromReaper{ false };  

    juce::AudioDeviceManager deviceManager;  // Spent an hour debugging because I put this after the DeviceRecorder, and it crashes without a trace. C++ is fun!
    std::unique_ptr<juce::XmlElement> deviceManagerLoadedState;
    bool deviceManagerLoaded{ false };
    DeviceRecorder deviceRecorder;

    PitchDetector pitchDetector;

    CustomLookAndFeel lookAndFeel;

    ReaperVST3Extensions reaperExtensions;
    const juce::String REAPER_FILE_PATH{ "P_EXT:FILE" };

    MTSClient* mtsClient{ nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JustaSampleAudioProcessor)
};
