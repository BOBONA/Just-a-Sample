/*
  ==============================================================================

    FilterResponse.h
    Created: 2 Jan 2024 10:40:21pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "CustomComponent.h"
#include "CustomSamplerSound.h"
#include "BandEQ.h"

class FilterResponse : public CustomComponent
{
public:
    FilterResponse(juce::AudioProcessorValueTreeState& apvts, int sampleRate);
    ~FilterResponse() override;

    void setSampleRate(int sampleRate);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    const static int startFreq{ 20 };
    const static int endFreq{ 20000 };

    CustomSamplerSound samplerSound;
    BandEQ eq;

    juce::AudioBuffer<float> _b;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterResponse)
};
