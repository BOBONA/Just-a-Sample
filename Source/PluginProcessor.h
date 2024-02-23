#pragma once

#include <JuceHeader.h>

#include "utilities/readerwriterqueue/readerwriterqueue.h"
#include "RubberBandStretcher.h"
#include "CustomLookAndFeel.h"
#include "sampler/CustomSamplerVoice.h"
#include "sampler/CustomSamplerSound.h"
#include "utilities/PitchDetector.h"

struct RecordingBufferChange
{
    enum RecordingBufferChangeType
    {
        CLEAR,
        ADD
    };

    RecordingBufferChange(RecordingBufferChangeType type, AudioBuffer<float> buffer = {}) :
        type(type),
        addedBuffer(buffer)
    {
    }

    RecordingBufferChangeType type;
    AudioBuffer<float> addedBuffer;
};

class JustaSampleAudioProcessor  : public AudioProcessor, public ValueTree::Listener, public AudioProcessorValueTreeState::Listener, 
    public Thread::Listener, public AudioIODeviceCallback
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    JustaSampleAudioProcessor();
    ~JustaSampleAudioProcessor() override;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // From AudioIODeviceCallback
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels, float* const* outputChannelData, int numOutputChannels, int numSamples, const AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void flushAccumulatedBuffer(); // this is since the GUI can't keep up unless the callbacks are accumulated
    void recordingFinished();

    /** Creates the plugin's parameter layout */
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** Whether the processor can handle a filePath's extension */
    bool canLoadFileExtension(const String& filePath);
    
    /** Load a file/sample and reset to default parameters, intended for when a new sample is loaded not from preset */
    /** reloadSample reloads whatever is in the sample buffer, tries is used for repeated prompting if necessary */
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

    /** Bounds checking for the loop start and end portions */
    void updateLoopStartPortionBounds();
    void updateLoopEndPortionBounds();
    int visibleSamples() const;

    void setProperLatency();
    void setProperLatency(PluginParameters::PLAYBACK_MODES mode);

    /** Detects the pitch of the current sample bounds and sets the tuning parameters */
    bool pitchDetectionRoutine(int startSample, int endSample);
    void exitSignalSent() override;

    const var& p(Identifier identifier) const
    {
        return apvts.state.getProperty(identifier);
    }

    Value pv(Identifier identifier)
    {
        return apvts.state.getPropertyAsValue(identifier, apvts.undoManager);
    }

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

    bool shouldRecord{ false };
    bool isRecording{ false };
    moodycamel::ReaderWriterQueue<RecordingBufferChange, 16384> recordingBufferQueue{ 10 }; // this is to give the info over to the GUI thread
    int recordingSampleRate;
    int recordingSize{ 0 };

private:
    Synthesiser synth;

    WildcardFileFilter fileFilter;
    std::unique_ptr<AudioFormatReader> formatReader;

    AudioBuffer<float> accumulatingRecordingBuffer; // Since the GUI thread can't keep up unless the callbacks are accumulated
    int accumulatingRecordingSize{ 0 };
    std::vector<std::unique_ptr<AudioBuffer<float>>> recordingBufferList;

    PitchDetector pitchDetector;

    CustomLookAndFeel lookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustaSampleAudioProcessor)
};