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
/*
*/
class SampleComponent  : public juce::Component
{
public:
    SampleComponent(juce::Array<int>& voicePositions);
    ~SampleComponent() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;

    void setSample(juce::AudioBuffer<float>& sample);
private:
    juce::AudioBuffer<float>* sample{ nullptr };

    SamplePainter painter;
    juce::Array<int>& voicePositions;
    juce::Rectangle<int> sampleArea;

    SampleNavigator navigator;

    CustomLookAndFeel& lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleComponent)
};
