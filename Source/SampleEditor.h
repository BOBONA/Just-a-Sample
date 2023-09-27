/*
  ==============================================================================

    SampleComponent.h
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SamplePainter.h"
#include "SampleNavigator.h"

//==============================================================================
class SampleEditorOverlay : public juce::Component
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

    CustomLookAndFeel& lnf;
};

//==============================================================================
class SampleEditor : public juce::Component
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
    
    CustomLookAndFeel& lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};
