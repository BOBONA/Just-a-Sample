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
#include "gin/gin_distortion.h"

/** This is a simple Distortion class that wraps around gin::AirWindowsDistortion */
class Distortion final : public Effect
{
public:
    void initialize(int numChannels, int fxSampleRate) override
    {
        int numEffects = numChannels / 2 + numChannels % 2;
        channelDistortions.resize(numEffects);
        for (int ch = 0; ch < numEffects; ch++)
        {
            channelDistortions[ch] = std::make_unique<gin::AirWindowsDistortion>();
            channelDistortions[ch]->setSampleRate(fxSampleRate);
        }
    }

    void updateParams(float density, float highpass, float mix)
    {
        float mappedDensity = density >= 0.f ? juce::jmap<float>(density, 0.2f, 1.f) : juce::jmap<float>(density, -0.5f, 0.f, 0.f, 0.2f);

        // We try to keep the gain of the distortion constant:
        // This is a sigmoid function found manually from graphing the output of the distortion. When the mapped density < 0.2f, 
        // a different function needs to be used, since the distortion actually behaves differently according to that threshold.
        // Note that the gain parameter only applies if it's less than 1.f, so we need to increase the gain ourselves after processing.
        float gainChange = mappedDensity >= 0.2f ? (0.2f * (1.f + expf(-7.f * (mappedDensity - 0.5f)))) : 1.f; 
        postGain = mappedDensity < 0.2f ? 1.f / (4.f * mappedDensity + 0.2f) : 1.f;

        for (const auto& channelDistortion : channelDistortions)
            channelDistortion->setParams(mappedDensity, highpass, gainChange, mix);
    }

    void updateParams(const SamplerParameters& sampleSound, bool) override
    {
        updateParams(
            sampleSound.distortionDensity->get(), 
            sampleSound.distortionHighpass->get(), 
            sampleSound.distortionMix->get()
        );
    }

    void process(juce::AudioBuffer<float>& buffer, int numSamples, int startSample=0) override
    {
        bool lastIsMono = buffer.getNumChannels() % 2 == 1;
        for (int ch = 0; ch < buffer.getNumChannels(); ch += 2)
        {
            // Note that I modified the gin header to make this more straightforward
            if (lastIsMono && ch == buffer.getNumChannels() - 1)
                channelDistortions[ch / 2]->process(buffer.getWritePointer(ch, startSample), buffer.getWritePointer(ch, startSample), numSamples);
            else
                channelDistortions[ch / 2]->process(buffer.getWritePointer(ch, startSample), buffer.getWritePointer(ch + 1, startSample), numSamples);
        }
        buffer.applyGain(startSample, numSamples, postGain);
    }

private:
    std::vector<std::unique_ptr<gin::AirWindowsDistortion>> channelDistortions;
    float postGain{ 0. };
};