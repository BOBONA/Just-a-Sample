/*
  ==============================================================================

    SampleComponent.h
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CustomComponent.h"
#include "SamplePainter.h"
#include "SampleNavigator.h"

//==============================================================================
class SampleEditorOverlay : public CustomComponent, public juce::Value::Listener
{
public:
    SampleEditorOverlay(APVTS& apvts, juce::Array<int>& voicePositions);
    ~SampleEditorOverlay() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void valueChanged(juce::Value& value) override;

    float sampleToPosition(int sample);
    int positionToSample(float position);

    void setSample(juce::AudioBuffer<float>& sample);
private:
    int painterWidth{ 0 };

    juce::AudioBuffer<float>* sample{ nullptr };
    juce::Value viewStart, viewStop;
    juce::Value sampleStart, sampleEnd;
    juce::Array<int>& voicePositions;
};

//==============================================================================
class SampleEditor : public CustomComponent, public juce::ValueTree::Listener
{
public:
    SampleEditor(APVTS& apvts, juce::Array<int>& voicePositions);
    ~SampleEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    void updateSamplePosition();
    void setSample(juce::AudioBuffer<float>& sample, bool resetUI);
private:
    APVTS& apvts;

    SamplePainter painter;
    SampleEditorOverlay overlay;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};
