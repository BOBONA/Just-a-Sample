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
class SampleEditorOverlay : public CustomComponent
{
public:
    SampleEditorOverlay(juce::Array<int>& voicePositions);
    ~SampleEditorOverlay() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void setSample(juce::AudioBuffer<float>& sample);
private:
    juce::AudioBuffer<float>* sample{ nullptr };
    juce::Array<int>& voicePositions;
};

//==============================================================================
class SampleEditor : public CustomComponent
{
public:
    SampleEditor(juce::Array<int>& voicePositions);
    ~SampleEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void updateSamplePosition();
    void setSample(juce::AudioBuffer<float>& sample);
private:
    SamplePainter painter;
    SampleEditorOverlay overlay;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};
