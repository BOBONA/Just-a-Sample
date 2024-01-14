/*
  ==============================================================================

    Chorus.h
    Created: 4 Jan 2024 7:22:36pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "Effect.h"

class Chorus : public Effect
{
public:
    Chorus(int expectedBlockSize=MAX_BLOCK_SIZE) : expectedBlockSize(expectedBlockSize)
    {
    }

    void initialize(int numChannels, int fxSampleRate)
    {
        juce::dsp::ProcessSpec processSpec{};
        processSpec.numChannels = numChannels;
        processSpec.sampleRate = fxSampleRate;
        processSpec.maximumBlockSize = expectedBlockSize;

        chorus.reset();
        chorus.prepare(processSpec);
    }

    void updateParams(CustomSamplerSound& sampleSound)
    {
        chorus.setRate(sampleSound.chorusRate.getValue());
        chorus.setDepth(sampleSound.chorusDepth.getValue());
        chorus.setFeedback(sampleSound.chorusFeedback.getValue());
        chorus.setCentreDelay(sampleSound.chorusCenterDelay.getValue());
        chorus.setMix(sampleSound.chorusMix.getValue());
    }

    void process(juce::AudioBuffer<float>& buffer, int numSamples, int startSample = 0)
    {
        while (numSamples > 0)
        {
            juce::dsp::AudioBlock<float> block{ buffer.getArrayOfWritePointers(), size_t(buffer.getNumChannels()), size_t(startSample), juce::jmin<size_t>(MAX_BLOCK_SIZE, numSamples) };
            juce::dsp::ProcessContextReplacing<float> context{ block };
            chorus.process(context);
            startSample += MAX_BLOCK_SIZE;
            numSamples -= MAX_BLOCK_SIZE;
        }
    }

private:
    const static int MAX_BLOCK_SIZE{ 1024 };

    juce::dsp::Chorus<float> chorus;
    int expectedBlockSize;
};