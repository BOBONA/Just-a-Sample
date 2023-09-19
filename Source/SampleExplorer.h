/*
  ==============================================================================

    SampleExplorer.h
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SamplePainter.h"

//==============================================================================
/*
*/
class SampleExplorer  : public juce::Component
{
public:
    SampleExplorer(juce::Array<int>& voicePositions);
    ~SampleExplorer() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setSample(juce::AudioBuffer<float>& sample);
private:
    SamplePainter painter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleExplorer)
};
