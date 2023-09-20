/*
  ==============================================================================

    SamplePainter.h
    Created: 19 Sep 2023 3:02:14pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CustomLookAndFeel.h"

//==============================================================================
/*
*/
class SamplePainter  : public juce::Component
{
public:
    SamplePainter();
    ~SamplePainter() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void updatePath();
    void setSample(juce::AudioBuffer<float>& sample);
private:
    juce::AudioBuffer<float>* sample{ nullptr };
    juce::Path path;

    CustomLookAndFeel& lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplePainter)
};
