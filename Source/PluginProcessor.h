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
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    /** Set the plugin's latency according to processing parameters (necessary for preprocessing options) */
    void setProperLatency();

    /** This signature is necessary for use in the APVTS callback */
    void setProperLatency(PluginParameters::PLAYBACK_MODES mode);
   
    /** Whether the processor can handle a filePath's extension */
    bool canLoadFileExtension(const String& filePath);

    /** Load a file/sample and reset to default parameters, intended for when a new sample is loaded not from preset
      * reloadSample reloads whatever is in the sample buffer, tries is used for repeated prompting if necessary */
    bool loadSampleAndReset(const String& path, bool reloadSample = false, bool makeNewFormatReader = true, int tries = 0);

    /** Load a file, updates the sampler, returns whether the file was loaded successfully */
    bool loadFile(const String& path, bool makeNewFormatReader = true, int expectedSampleLength = -1);

    /** Whether the sample buffer is too large to be stored in the plugin data */
    bool sampleBufferNeedsReference() const;

    /** Open a file chooser */
    void openFileChooser(const String& message, int flags, std::function<void(const FileChooser&)> callback, bool wavOnly = false);

    /** Reset the sampler voices */
    void resetSamplerVoices();
    void haltVoices();

    /** Updates the sampler with a new AudioBuffer */
    void updateSamplerSound(AudioBuffer<float>& sample);

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
    DeviceRecorder& getRecorder() { return deviceRecorder; }

    //==============================================================================
    /** Detects the pitch of the current sample bounds and sets the tuning parameters */
    bool pitchDetectionRoutine(int startSample, int endSample);
    void exitSignalSent() override;

    //==============================================================================
    const var& p(Identifier identifier) const { return apvts.state.getProperty(identifier); }
    Value pv(Identifier identifier) { return apvts.state.getPropertyAsValue(identifier, apvts.undoManager); }

    juce::AudioProcessorValueTreeState apvts;
    juce::UndoManager undoManager;
    AudioFormatManager formatManager;
    AudioDeviceManager deviceManager;

    AudioBuffer<float> sampleBuffer;
    double bufferSampleRate{ 0. };
    Array<CustomSamplerVoice*> samplerVoices;

    String samplePath;
    bool usingFileReference{ true };
    std::unique_ptr<FileChooser> fileChooser;

    // this flag helps the editor differentiate between a user loading a file and a preset, which is needed because the editor handles the sample view state
    bool resetParameters{ false };
    int editorWidth, editorHeight;

private:
    Synthesiser synth;

    WildcardFileFilter fileFilter;
    std::unique_ptr<AudioFormatReader> formatReader;

    DeviceRecorder deviceRecorder;

    PitchDetector pitchDetector;

    CustomLookAndFeel lookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JustaSampleAudioProcessor)
};