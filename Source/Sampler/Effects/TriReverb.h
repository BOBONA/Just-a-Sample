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

/* This is a simple Effect class that wraps around the available reverb effects in the project's dependencies. Note that the parameter ranges need to be manually specified in PluginParameters.h */
class TriReverb : public Effect
{
public:
    TriReverb()
    {
    }

    ~TriReverb()
    {
    }

    void initialize(int numChannels, int fxSampleRate)
    {
        int numEffects = numChannels / 2 + numChannels % 2;
        switch (PluginParameters::REVERB_TYPE)
        {
        case PluginParameters::JUCE:
            channelJuceReverbs.resize(numEffects);
            for (int ch = 0; ch < numEffects; ch++)
            {
                channelJuceReverbs[ch] = std::make_unique<juce::Reverb>();
                channelJuceReverbs[ch]->reset();
                channelJuceReverbs[ch]->setSampleRate(fxSampleRate);
            }
            break;
        case PluginParameters::GIN_SIMPLE:
            channelGinReverbs.resize(numEffects);
            for (int ch = 0; ch < numEffects; ch++)
            {
                channelGinReverbs[ch] = std::make_unique<gin::SimpleVerb>();
                channelGinReverbs[ch]->setSampleRate(float(fxSampleRate));
                channelGinReverbs[ch]->setParameters(0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f);
            }
            break;
        case PluginParameters::GIN_PLATE:
            channelPlateReverbs.resize(numEffects);
            for (int ch = 0; ch < numEffects; ch++)
            {
                channelPlateReverbs[ch] = std::make_unique<gin::PlateReverb<float, int>>();
                channelPlateReverbs[ch]->reset();
                channelPlateReverbs[ch]->setSampleRate(float(fxSampleRate));
            }
            break;
        }
    }

    // the intended ranges of these values are in PluginParameters.h
    void updateParams(float size, float damping, float predelay, float lows, float highs, float mix)
    {
        for (int ch = 0; ch < channelGinReverbs.size(); ch++)
        {
            channelGinReverbs[ch]->setParameters(
                juce::jmap<float>(size, PluginParameters::REVERB_SIZE_RANGE.getStart(), PluginParameters::REVERB_SIZE_RANGE.getEnd(), 0.f, 1.f),
                juce::jmap<float>(damping, PluginParameters::REVERB_DAMPING_RANGE.getStart(), PluginParameters::REVERB_DAMPING_RANGE.getEnd(), 0.f, 1.f),
                float(sqrtf(predelay / 250.f)), // conversions to counteract the faders in this algorithm
                juce::jmap<float>(highs, PluginParameters::REVERB_HIGHS_RANGE.getStart(), PluginParameters::REVERB_HIGHS_RANGE.getEnd(), 0.3f, 1.f),
                1.f - juce::jmap<float>(lows, PluginParameters::REVERB_LOWS_RANGE.getStart(), PluginParameters::REVERB_LOWS_RANGE.getEnd(), 0.3f, 1.f),
                mix,
                1.f - mix
            );
        }
    }

    void updateParams(CustomSamplerSound& sampleSound)
    {
        switch (PluginParameters::REVERB_TYPE)
        {
        case PluginParameters::JUCE:
        {
            juce::Reverb::Parameters reverbParams;
            reverbParams.wetLevel = sampleSound.reverbMix.getValue();
            reverbParams.dryLevel = 1 - reverbParams.wetLevel;
            reverbParams.roomSize = sampleSound.reverbSize.getValue();
            reverbParams.damping = sampleSound.reverbDamping.getValue();
            reverbParams.width = sampleSound.reverbLows.getValue();
            reverbParams.freezeMode = sampleSound.reverbHighs.getValue();
            for (int ch = 0; ch < channelJuceReverbs.size(); ch++)
            {
                channelJuceReverbs[ch]->setParameters(reverbParams);
            }
            break;
        }
        case PluginParameters::GIN_SIMPLE:
            updateParams(
                sampleSound.reverbSize.getValue(),
                sampleSound.reverbDamping.getValue(),
                sampleSound.reverbPredelay.getValue(),
                sampleSound.reverbLows.getValue(),
                sampleSound.reverbHighs.getValue(),
                sampleSound.reverbMix.getValue()
            );
            break;
        case PluginParameters::GIN_PLATE:
            for (int ch = 0; ch < channelPlateReverbs.size(); ch++)
            {
                channelPlateReverbs[ch]->setMix(sampleSound.reverbMix.getValue());
                channelPlateReverbs[ch]->setSize(sampleSound.reverbSize.getValue());
                channelPlateReverbs[ch]->setDamping(sampleSound.reverbDamping.getValue());
                channelPlateReverbs[ch]->setLowpass(sampleSound.reverbLows.getValue());
                channelPlateReverbs[ch]->setDecay(sampleSound.reverbHighs.getValue());
                channelPlateReverbs[ch]->setPredelay(sampleSound.reverbPredelay.getValue());
            }
            break;
        }
    }

    void process(juce::AudioBuffer<float>& buffer, int numSamples, int startSample = 0)
    {
        bool lastIsMono = buffer.getNumChannels() % 2 == 1;
        for (int ch = 0; ch < buffer.getNumChannels(); ch += 2)
        {
            switch (PluginParameters::REVERB_TYPE)
            {
            case PluginParameters::JUCE:
                if (lastIsMono && ch == buffer.getNumChannels() - 1)
                {
                    channelJuceReverbs[ch / 2]->processMono(buffer.getWritePointer(ch, startSample), numSamples);
                }
                else
                {
                    channelJuceReverbs[ch / 2]->processStereo(
                        buffer.getWritePointer(ch, startSample), 
                        buffer.getWritePointer(ch + 1, startSample), numSamples);
                }
                break;
            case PluginParameters::GIN_SIMPLE:
                // I modified the header of the process method to work easier with this code
                if (lastIsMono && ch == buffer.getNumChannels() - 1)
                {
                    channelGinReverbs[ch / 2]->process(
                        buffer.getReadPointer(ch, startSample), buffer.getReadPointer(ch, startSample),
                        buffer.getWritePointer(ch, startSample), buffer.getWritePointer(ch, startSample), numSamples);
                }
                else
                {
                    channelGinReverbs[ch / 2]->process(
                        buffer.getReadPointer(ch, startSample), buffer.getReadPointer(ch + 1, startSample),
                        buffer.getWritePointer(ch, startSample), buffer.getWritePointer(ch + 1, startSample), numSamples);
                }
                break;
            case PluginParameters::GIN_PLATE:
                if (lastIsMono && ch == buffer.getNumChannels() - 1)
                {
                    channelPlateReverbs[ch / 2]->process(buffer.getWritePointer(ch, startSample), buffer.getWritePointer(ch, startSample), numSamples);
                }
                else
                {
                    channelPlateReverbs[ch / 2]->process(buffer.getWritePointer(ch, startSample), buffer.getWritePointer(ch + 1, startSample), numSamples);
                }
                break;
            }
        }
    }

private:
    std::vector<std::unique_ptr<juce::Reverb>> channelJuceReverbs;
    std::vector<std::unique_ptr<gin::SimpleVerb>> channelGinReverbs;
    std::vector<std::unique_ptr<gin::PlateReverb<float, int>>> channelPlateReverbs;
};