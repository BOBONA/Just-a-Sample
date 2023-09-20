/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SampleComponent.h"
#include "CustomLookAndFeel.h"

//==============================================================================
/**
*/
class JustaSampleAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer, public juce::FileDragAndDropTarget
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
private:
    JustaSampleAudioProcessor& processor;

    SampleComponent sampleComponent;
    juce::Array<int> voicePositions;

    CustomLookAndFeel& lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustaSampleAudioProcessorEditor)
};
