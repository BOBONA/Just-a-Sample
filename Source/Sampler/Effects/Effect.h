/*
  ==============================================================================

    Effect.h
    Created: 30 Dec 2023 8:35:28pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../CustomSamplerSound.h"

class Effect
{
public:
    virtual ~Effect() {};
    virtual void initialize(int numChannels, int fxSampleRate) = 0;
    virtual void updateParams(CustomSamplerSound& sampleSound) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer, int numSamples, int startSample=0) = 0;
};