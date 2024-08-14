/*
  ==============================================================================

    Reverb.h
    Created: 30 Dec 2023 8:38:55pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "Effect.h"

/** This is a simple Effect class that wraps around Gin's SimpleVerb implementation */
class Reverb final : public Effect
{
public:
    Reverb() = default;

    void initialize(int numChannels, int fxSampleRate) override
    {
        int numEffects = numChannels / 2 + numChannels % 2;
        channelGinReverbs.resize(numEffects);
        for (int ch = 0; ch < numEffects; ch++)
        {
            channelGinReverbs[ch] = std::make_unique<gin::SimpleVerb>();
            channelGinReverbs[ch]->setSampleRate(float(fxSampleRate));
            channelGinReverbs[ch]->setParameters(0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f);
        }
    }

    // The intended ranges of these values are in PluginParameters.h
    void updateParams(float size, float damping, float predelay, float lows, float highs, float mix) const
    {
        for (const auto& channelGinReverb : channelGinReverbs)
        {
            channelGinReverb->setParameters(
                juce::jmap<float>(size, PluginParameters::REVERB_SIZE_RANGE.getStart(), PluginParameters::REVERB_SIZE_RANGE.getEnd(), 0.f, 1.f),
                juce::jmap<float>(damping, PluginParameters::REVERB_DAMPING_RANGE.getStart(), PluginParameters::REVERB_DAMPING_RANGE.getEnd(), 0.f, 1.f),
                float(sqrtf(predelay / 250.f)),  // conversions to counteract the faders in this algorithm
                juce::jmap<float>(highs, PluginParameters::REVERB_HIGHS_RANGE.getStart(), PluginParameters::REVERB_HIGHS_RANGE.getEnd(), 0.3f, 1.f),
                1.f - juce::jmap<float>(lows, PluginParameters::REVERB_LOWS_RANGE.getStart(), PluginParameters::REVERB_LOWS_RANGE.getEnd(), 0.3f, 1.f),
                mix,
                1.f - mix
            );
        }
    }

    void updateParams(const SamplerParameters& sampleSound) override
    {
        updateParams(
            sampleSound.reverbSize->get(),
            sampleSound.reverbDamping->get(),
            sampleSound.reverbPredelay->get(),
            sampleSound.reverbLows->get(),
            sampleSound.reverbHighs->get(),
            sampleSound.reverbMix->get()
        );
    }

    void process(juce::AudioBuffer<float>& buffer, int numSamples, int startSample = 0) override
    {
        bool lastIsMono = buffer.getNumChannels() % 2 == 1;
        for (int ch = 0; ch < buffer.getNumChannels(); ch += 2)
        {
            // I modified the header of the process method to work easier with this code, you'll need to do the same to get it to compile
            // void SimpleVerb::process (const float* in1, const float* in2, float* out1, float* out2, int numSamples)
            if (lastIsMono && ch == buffer.getNumChannels() - 1)
                channelGinReverbs[ch / 2]->process(
                    buffer.getReadPointer(ch, startSample), buffer.getReadPointer(ch, startSample),
                    buffer.getWritePointer(ch, startSample), buffer.getWritePointer(ch, startSample), numSamples);
            else
                channelGinReverbs[ch / 2]->process(
                    buffer.getReadPointer(ch, startSample), buffer.getReadPointer(ch + 1, startSample),
                    buffer.getWritePointer(ch, startSample), buffer.getWritePointer(ch + 1, startSample), numSamples);
        }
    }

private:
    std::vector<std::unique_ptr<gin::SimpleVerb>> channelGinReverbs;
};