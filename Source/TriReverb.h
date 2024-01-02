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
class TriReverb : Effect
{
public:
    TriReverb()
    {
    }

    ~TriReverb()
    {
    }

    void initialize(int numChannels, int sampleRate)
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
                channelJuceReverbs[ch]->setSampleRate(sampleRate);
            }
            break;
        case PluginParameters::GIN_SIMPLE:
            channelGinReverbs.resize(numEffects);
            for (int ch = 0; ch < numEffects; ch++)
            {
                channelGinReverbs[ch] = std::make_unique<gin::SimpleVerb>();
                channelGinReverbs[ch]->setSampleRate(sampleRate);
                channelGinReverbs[ch]->setParameters(0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f);
            }
            break;
        case PluginParameters::GIN_PLATE:
            channelPlateReverbs.resize(numEffects);
            for (int ch = 0; ch < numEffects; ch++)
            {
                channelPlateReverbs[ch] = std::make_unique<gin::PlateReverb<float, int>>();
                channelPlateReverbs[ch]->reset();
                channelPlateReverbs[ch]->setSampleRate(sampleRate);
            }
            break;
        }
    }

    void updateParams(CustomSamplerSound& sampleSound)
    {
        switch (PluginParameters::REVERB_TYPE)
        {
        case PluginParameters::JUCE:
        {
            Reverb::Parameters reverbParams;
            reverbParams.wetLevel = sampleSound.reverbMix.getValue();
            reverbParams.dryLevel = 1 - reverbParams.wetLevel;
            reverbParams.roomSize = sampleSound.reverbSize.getValue();
            reverbParams.damping = sampleSound.reverbDamping.getValue();
            reverbParams.width = sampleSound.reverbParam1.getValue();
            reverbParams.freezeMode = sampleSound.reverbParam2.getValue();
            for (int ch = 0; ch < channelJuceReverbs.size(); ch++)
            {
                channelJuceReverbs[ch]->setParameters(reverbParams);
            }
            break;
        }
        case PluginParameters::GIN_SIMPLE:
            for (int ch = 0; ch < channelGinReverbs.size(); ch++)
            {
                channelGinReverbs[ch]->setParameters(
                    sampleSound.reverbSize.getValue(),
                    sampleSound.reverbDamping.getValue(),
                    sampleSound.reverbParam3.getValue(),
                    sampleSound.reverbParam1.getValue(),
                    sampleSound.reverbParam2.getValue(),
                    sampleSound.reverbMix.getValue(),
                    1 - float(sampleSound.reverbMix.getValue()));
            }
            break;
        case PluginParameters::GIN_PLATE:
            for (int ch = 0; ch < channelPlateReverbs.size(); ch++)
            {
                channelPlateReverbs[ch]->setMix(sampleSound.reverbMix.getValue());
                channelPlateReverbs[ch]->setSize(sampleSound.reverbSize.getValue());
                channelPlateReverbs[ch]->setDamping(sampleSound.reverbDamping.getValue());
                channelPlateReverbs[ch]->setLowpass(sampleSound.reverbParam1.getValue());
                channelPlateReverbs[ch]->setDecay(sampleSound.reverbParam2.getValue());
                channelPlateReverbs[ch]->setPredelay(sampleSound.reverbParam3.getValue());
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
    std::vector<std::unique_ptr<Reverb>> channelJuceReverbs;
    std::vector<std::unique_ptr<gin::SimpleVerb>> channelGinReverbs;
    std::vector<std::unique_ptr<gin::PlateReverb<float, int>>> channelPlateReverbs;
};