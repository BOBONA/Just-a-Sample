/*
  ==============================================================================

    SamplePainter.h
    Created: 19 Sep 2023 3:02:14pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../ComponentUtils.h"

class SamplePainter  : public CustomComponent
{
public:
    SamplePainter();
    ~SamplePainter() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void updatePath();
    void setSample(juce::AudioBuffer<float>& sample);
    void setSample(juce::AudioBuffer<float>& sample, int start, int stop);
    void setSampleView(int start, int stop);
private:
    juce::AudioBuffer<float>* sample{ nullptr };
    int start{ 0 }, stop{ 0 };
    juce::Path path;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplePainter)
};
