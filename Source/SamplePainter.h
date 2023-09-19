/*
  ==============================================================================

    SamplePainter.h
    Created: 19 Sep 2023 3:02:14pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class SamplePainter  : public juce::Component
{
public:
    SamplePainter(juce::Array<int>& voicePositions);
    ~SamplePainter() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setSample(juce::AudioBuffer<float>& sample);

private:
    juce::AudioBuffer<float>* sample{ nullptr };
    juce::Array<int>& voicePositions;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplePainter)
};
