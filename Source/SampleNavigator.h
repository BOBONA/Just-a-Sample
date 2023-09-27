/*
  ==============================================================================

    SampleNavigator.h
    Created: 19 Sep 2023 4:41:12pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CustomComponent.h"
#include "SamplePainter.h"

//==============================================================================
/*
*/
class SampleNavigatorOverlay : public CustomComponent
{
public:
    SampleNavigatorOverlay(juce::Array<int>& voicePositions);
    ~SampleNavigatorOverlay() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void setSample(juce::AudioBuffer<float>& sample);
private:
    juce::AudioBuffer<float>* sample{ nullptr };
    juce::Array<int>& voicePositions;
};

//==============================================================================
class SampleNavigator  : public CustomComponent
{
public:
    SampleNavigator(juce::Array<int>& voicePositions);
    ~SampleNavigator() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void updateSamplePosition();
    void setSample(juce::AudioBuffer<float>& sample);
private:
    SamplePainter painter;
    SampleNavigatorOverlay overlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleNavigator)
};
