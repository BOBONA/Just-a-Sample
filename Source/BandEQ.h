/*
  ==============================================================================

    BandEQ.h
    Created: 2 Jan 2024 5:02:27pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "Effect.h"

using namespace juce::dsp;

class BandEQ : Effect
{
public:
    void initialize(int numChannels, int sampleRate)
    {
        this->sampleRate = sampleRate;
        filterChain.reset();
        ProcessSpec spec{};
        spec.numChannels = numChannels;
        spec.sampleRate = sampleRate;
        filterChain.prepare(spec); // Not sure if Filters use this spec
    }

    void updateParams(float lowFreq, float highFreq, float lowGain, float midGain, float highGain)
    {
        // the values should already be in a valid range but sometimes wierd stuff happens when the plugin first loads...
        auto lowFreqRange = PluginParameters::EQ_LOW_FREQ_RANGE;
        auto highFreqRange = PluginParameters::EQ_HIGH_FREQ_RANGE;
        auto coeffLow = IIR::Coefficients<float>::makeLowShelf(sampleRate, jlimit<float>(lowFreqRange.getStart(), lowFreqRange.getEnd(), lowFreq), Q, Decibels::decibelsToGain(lowGain));
        auto coeffMid1 = IIR::Coefficients<float>::makeHighShelf(sampleRate, jlimit<float>(lowFreqRange.getStart(), lowFreqRange.getEnd(), lowFreq), Q, Decibels::decibelsToGain(midGain));
        auto coeffMid2 = IIR::Coefficients<float>::makeHighShelf(sampleRate, jlimit<float>(highFreqRange.getStart(), highFreqRange.getEnd(), highFreq), Q, Decibels::decibelsToGain(-midGain));
        auto coeffHigh = IIR::Coefficients<float>::makeHighShelf(sampleRate, jlimit<float>(highFreqRange.getStart(), highFreqRange.getEnd(), highFreq), Q, Decibels::decibelsToGain(highGain));
        *filterChain.get<0>().state = *coeffLow;
        *filterChain.get<1>().state = *coeffMid1;
        *filterChain.get<2>().state = *coeffMid2;
        *filterChain.get<3>().state = *coeffHigh;
    }

    void updateParams(CustomSamplerSound& samplerSound)
    {
        updateParams(
            samplerSound.eqLowFreq.getValue(), samplerSound.eqHighFreq.getValue(), 
            samplerSound.eqLowGain.getValue(), samplerSound.eqMidGain.getValue(), samplerSound.eqHighGain.getValue()
        );
    }
    
    void process(juce::AudioBuffer<float>& buffer, int numSamples, int startSample=0)
    {
        AudioBlock<float> block{ buffer.getArrayOfWritePointers(), size_t(buffer.getNumChannels()), size_t(startSample), size_t(numSamples) };
        ProcessContextReplacing<float> context{ block };
        filterChain.process(context);
    }

    juce::Array<double> getMagnitudeForFrequencyArray(juce::Array<double> frequencies)
    {
        std::array<Filter*, 4> filters{
            &filterChain.get<0>(),
            &filterChain.get<1>(),
            &filterChain.get<2>(),
            &filterChain.get<3>()
        };
        juce::AudioBuffer<double> magnitudes{1, frequencies.size()};
        magnitudes.clear();
        bool empty{ true };
        for (const auto& filter : filters)
        {
            juce::AudioBuffer<double> temp{ 1, frequencies.size()};
            filter->state->getMagnitudeForFrequencyArray(frequencies.getRawDataPointer(), temp.getWritePointer(0), frequencies.size(), sampleRate);
            if (empty)
            {
                magnitudes.addFrom(0, 0, temp.getReadPointer(0), frequencies.size());
                empty = false;
            }
            else
            {
                for (int i = 0; i < temp.getNumSamples(); i++)
                {
                    magnitudes.setSample(0, i, magnitudes.getSample(0, i) * temp.getSample(0, i));
                }
            }
        }
        return juce::Array<double>{magnitudes.getReadPointer(0), frequencies.size()};
    }

private:
    using Filter = ProcessorDuplicator<IIR::Filter<float>, IIR::Coefficients<float>>;
    using FilterChain = ProcessorChain<Filter, Filter, Filter, Filter>;

    const float Q{ 0.6 }; // magic number I found for the response curve
    
    int sampleRate{ 0 };
    FilterChain filterChain;
};