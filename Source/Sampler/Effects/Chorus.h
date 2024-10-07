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

/** This is a simple wrapper around the JUCE Chorus class */
class Chorus final : public Effect
{
public:
    explicit Chorus(int expectedBlockSize=MAX_BLOCK_SIZE) : expectedBlockSize(expectedBlockSize) {}

    void initialize(int numChannels, int fxSampleRate) override
    {
        juce::dsp::ProcessSpec processSpec{ double(fxSampleRate), juce::uint32(expectedBlockSize), juce::uint32(numChannels) };

        chorus.reset();
        chorus.prepare(processSpec);
    }

    void updateParams(const SamplerParameters& sampleSound, bool realtime) override
    {
        chorus.setRate(sampleSound.chorusRate->get());
        chorus.setDepth(sampleSound.chorusDepth->get());
        chorus.setFeedback(sampleSound.chorusFeedback->get());
        if (!realtime)  // Center delay cannot be modulated
            chorus.setCentreDelay(juce::jmin<float>(sampleSound.chorusCenterDelay->get(), 99.9f));
        chorus.setMix(sampleSound.chorusMix->get());
    }

    void process(juce::AudioBuffer<float>& buffer, int numSamples, int startSample = 0) override
    {
        while (numSamples > 0)
        {
            juce::dsp::AudioBlock<float> block{ buffer.getArrayOfWritePointers(), size_t(buffer.getNumChannels()), size_t(startSample), size_t(juce::jmin(MAX_BLOCK_SIZE, numSamples)) };
            juce::dsp::ProcessContextReplacing<float> context{ block };
            chorus.process(context);
            startSample += MAX_BLOCK_SIZE;
            numSamples -= MAX_BLOCK_SIZE;
        }
    }

private:
    static constexpr int MAX_BLOCK_SIZE{ 1024 };

    juce::dsp::Chorus<float> chorus{};
    int expectedBlockSize;
};