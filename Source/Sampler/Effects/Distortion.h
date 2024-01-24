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
    void initialize(int numChannels, int fxSampleRate)
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
        float mappedDensity = density >= 0.f ? jmap<float>(density, 0.2f, 1.f) : jmap<float>(density, -0.5f, 0.f, 0.f, 0.2f);

        // This is a sigmoid function found from testing the response of the distortion algorithm. When the mapped density < 0.2f, 
        // a different function needs to be used, since the distortion actually behaves differently according to that threshold.
        // Note that the gain parameter only applies if it's less than 1.f, so we need to increase the gain ourselves after processing.
        float gainChange = mappedDensity >= 0.2f ? (0.2f * (1.f + expf(-7.f * (mappedDensity - 0.5f)))) : 1.f; 
        postGain = mappedDensity < 0.2f ? 1.f / (4.f * mappedDensity + 0.2f) : 1.f;
        for (int ch = 0; ch < channelDistortions.size(); ch++)
        {
            channelDistortions[ch]->setParams(mappedDensity, highpass, gainChange, mix);
        }
    }

    void updateParams(CustomSamplerSound& sampleSound)
    {
        updateParams(
            float(sampleSound.distortionDensity.getValue()), 
            float(sampleSound.distortionHighpass.getValue()), 
            float(sampleSound.distortionMix.getValue())
        );
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
        buffer.applyGain(startSample, numSamples, postGain);
    }

private:
    Array<float> testDensityResponse(AudioBuffer<float>& buffer, int numSamples, int startSample, bool print=false)
    {
        Array<float> output;
        for (float i = 0.f; i < 1.f; i += 0.01f)
        {
            initialize(1, 44100);
            updateParams(i, 0.f, 1.f);

            juce::AudioBuffer<float> copy{ 1, numSamples };
            copy.copyFrom(0, 0, buffer.getReadPointer(0, startSample), numSamples);
            float baseLevel = copy.getRMSLevel(0, 0, numSamples);
            process(copy, numSamples, 0);
            output.add(copy.getRMSLevel(0, 0, numSamples) / baseLevel);
        }

        if (print)
        {
            String out;
            for (int i = 0; i < output.size(); i++)
            {
                out << "(" << i / 100.f << ", " << output[i] << "), ";
            }
            DBG(out);
        }

        return output;
    }

    std::vector<std::unique_ptr<gin::AirWindowsDistortion>> channelDistortions;
    float postGain;
};