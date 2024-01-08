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
    void initialize(int numChannels, int sampleRate)
    {
        juce::dsp::ProcessSpec processSpec{};
        processSpec.numChannels = numChannels;
        processSpec.sampleRate = sampleRate;
        processSpec.maximumBlockSize = MAX_BLOCK_SIZE;

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
    const int MAX_BLOCK_SIZE{ 1024 };

    juce::dsp::Chorus<float> chorus;
};