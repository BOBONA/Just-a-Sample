#pragma once

#include <JuceHeader.h>

#include "RubberBandStretcher.h"
#include "CustomLookAndFeel.h"
#include "sampler/CustomSamplerVoice.h"
#include "sampler/CustomSamplerSound.h"
#include "utilities/PitchDetector.h"

class JustaSampleAudioProcessor  : public AudioProcessor, public ValueTree::Listener, public AudioProcessorValueTreeState::Listener, public Thread::Listener
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

    /** Creates the plugin's parameter layout */
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** Whether the processor can handle a filePath's extension */
    bool canLoadFileExtension(const String& filePath);
    
    /** Load a file and reset to default parameters, intended for when the user manually drags a file */
    void loadFileAndReset(const String& path);
    
    /** Load a file, updates the sampler, returns whether the file was loaded successfully */
    bool loadFile(const String& path);

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
    bool pitchDetectionRoutine();
    void exitSignalSent() override;

    var p(Identifier identifier) const
    {
        return apvts.state.getProperty(identifier);
    }

    Value pv(Identifier identifier)
    {
        return apvts.state.getPropertyAsValue(identifier, apvts.undoManager);
    }

    AudioBuffer<float>& getSample()
    {
        return sampleBuffer;
    }

    juce::Array<CustomSamplerVoice*>& getSynthVoices()
    {
        return samplerVoices;
    }

    juce::AudioProcessorValueTreeState apvts;
    juce::UndoManager undoManager;
    bool resetParameters{ false }; // a flag used to differentiate when a user loads a file versus a preset
    int editorWidth, editorHeight;

    bool isPitchDetecting{ false };
private:
    Synthesiser synth;

    AudioFormatManager formatManager;
    WildcardFileFilter fileFilter;
    std::unique_ptr<AudioFormatReader> formatReader;

    String samplePath;
    AudioBuffer<float> sampleBuffer;
    Array<CustomSamplerVoice*> samplerVoices;

    PitchDetector pitchDetector;

    CustomLookAndFeel lookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustaSampleAudioProcessor)
};