/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "CustomSamplerVoice.h"
#include "CustomSamplerSound.h"
#include "RubberBandStretcher.h"
#include "CustomLookAndFeel.h"

//==============================================================================
/**
*/
class JustaSampleAudioProcessor  : public juce::AudioProcessor, public juce::ValueTree::Listener
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    JustaSampleAudioProcessor();
    ~JustaSampleAudioProcessor() override;

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
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    bool canLoadFileExtension(const String& filePath);
    void loadFileAndReset(const String& path); // when the user drags a file the parameters should be reset
    bool loadFile(const String& path);
    void resetSamplerVoices();
    void updateSamplerSound(AudioBuffer<float>& sample);

    void updateProcessor();
    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

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
private:
    const int NUM_VOICES = 8;
    float BASE_FREQ = 523.25;
    Synthesiser synth;
    
    AudioFormatManager formatManager;
    WildcardFileFilter fileFilter;
    AudioFormatReader* formatReader{ nullptr };

    String samplePath;
    AudioBuffer<float> sampleBuffer;
    Array<CustomSamplerVoice*> samplerVoices;

    CustomLookAndFeel lookAndFeel;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustaSampleAudioProcessor)
};