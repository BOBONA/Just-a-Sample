/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SampleEditor.h"
#include "CustomLookAndFeel.h"
#include "PluginParameters.h"

//==============================================================================
/**
*/
class JustaSampleAudioProcessorEditor  : public AudioProcessorEditor, public Timer, public FileDragAndDropTarget, public ValueTree::Listener, public APVTS::Listener
{
public:
    JustaSampleAudioProcessorEditor (JustaSampleAudioProcessor&);
    ~JustaSampleAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // Inherited via FileDragAndDropTarget
    bool isInterestedInFileDrag(const String& file);
    bool isInterestedInFileDrag(const StringArray& files) override;
    void filesDropped(const StringArray& files, int x, int y) override;

    // Inherited via Timer
    void timerCallback() override;

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;
    void updateWorkingSample();
private:
    JustaSampleAudioProcessor& processor;
    Array<CustomSamplerVoice*>& synthVoices;
    bool currentlyPlaying{ false };

    Label fileLabel;
    ComboBox playbackOptions;
    Label isLoopingLabel;
    ToggleButton isLoopingButton;
    Label gainLabel;
    Slider masterGainSlider;
    bool gainChanged;

    SampleEditor sampleEditor;
    SampleNavigator sampleNavigator;

    APVTS::ComboBoxAttachment playbackOptionsAttachment;
    APVTS::ButtonAttachment loopToggleButtonAttachment;
    APVTS::SliderAttachment masterGainSliderAttachment;

    CustomLookAndFeel& lnf;

    OpenGLContext openGLContext;
    PluginHostType hostType;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustaSampleAudioProcessorEditor)

        // Inherited via Listener
        void parameterChanged(const String& parameterID, float newValue) override;
};
