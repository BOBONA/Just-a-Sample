/*
  ==============================================================================

    Distortion.h
    Created: 30 Dec 2023 8:39:04pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "Effect.h"

/* This is a simple Distortion class that wraps around gin::AirWindowsDistortion */
class Distortion : public Effect
{
public:
    void initialize(int numChannels, int sampleRate)
    {
        int numEffects = numChannels / 2 + numChannels % 2;
        channelDistortions.resize(numEffects);
        for (int ch = 0; ch < numEffects; ch++)
        {
            channelDistortions[ch] = std::make_unique<gin::AirWindowsDistortion>();
            channelDistortions[ch]->setSampleRate(sampleRate);
        }
    }

    void updateParams(CustomSamplerSound& sampleSound)
    {
        for (int ch = 0; ch < channelDistortions.size(); ch++)
        {
            channelDistortions[ch]->setParams(
                sampleSound.distortionDensity.getValue(),
                sampleSound.distortionHighpass.getValue(),
                sampleSound.distortionOutput.getValue(),
                sampleSound.distortionMix.getValue()
            );
        }
    }

    void process(juce::AudioBuffer<float>& buffer, int numSamples, int startSample=0)
    {
        bool lastIsMono = buffer.getNumChannels() % 2 == 1;
        for (int ch = 0; ch < buffer.getNumChannels(); ch += 2)
        {
            if (lastIsMono && ch == buffer.getNumChannels() - 1)
            {
                channelDistortions[ch / 2]->process(buffer.getWritePointer(ch, startSample), buffer.getWritePointer(ch, startSample), numSamples);
            }
            else
            {
                channelDistortions[ch / 2]->process(buffer.getWritePointer(ch, startSample), buffer.getWritePointer(ch + 1, startSample), numSamples);
            }
        }
    }

private:
    std::vector<std::unique_ptr<gin::AirWindowsDistortion>> channelDistortions;
};