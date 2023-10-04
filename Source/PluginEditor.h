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
class JustaSampleAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer, public juce::FileDragAndDropTarget, public juce::ValueTree::Listener
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
    void updateUI();
    void updateSample();
private:
    JustaSampleAudioProcessor& processor;

    SampleEditor sampleEditor;
    SampleNavigator sampleNavigator;
    juce::Array<int> voicePositions;
    bool resetUIParameters{ false };

    CustomLookAndFeel& lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustaSampleAudioProcessorEditor)
};
