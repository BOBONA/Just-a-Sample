/*
  ==============================================================================

    SampleNavigator.h
    Created: 19 Sep 2023 4:41:12pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SamplePainter.h"

//==============================================================================
/*
*/
class SampleNavigator  : public juce::Component
{
public:
    SampleNavigator();
    ~SampleNavigator() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SamplePainter samplePainter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleNavigator)
};
