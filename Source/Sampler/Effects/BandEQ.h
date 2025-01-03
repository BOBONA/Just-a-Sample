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

/** This is a sample 3-band EQ effect, inspired by the Kiloheart's free 3-Band EQ plugin. */
class BandEQ final : public Effect
{
public:
    void initialize(int numChannels, int fxSampleRate) override
    {
        sampleRate = fxSampleRate;

        juce::dsp::ProcessSpec spec{};
        spec.numChannels = numChannels;
        spec.sampleRate = sampleRate;
        filterChain.reset();
        filterChain.prepare(spec);
    }

    void updateParams(float lowFreq, float highFreq, float lowGain, float midGain, float highGain)
    {
        // This possibly has an issue where the frequencies are outside of range on plugin initialization
        auto coeffLow = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, lowFreq, Q, juce::Decibels::decibelsToGain(lowGain));
        auto coeffMid1 = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, lowFreq, Q, juce::Decibels::decibelsToGain(midGain));
        auto coeffMid2 = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, highFreq, Q, juce::Decibels::decibelsToGain(-midGain));
        auto coeffHigh = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, highFreq, Q, juce::Decibels::decibelsToGain(highGain));
        *filterChain.get<0>().state = *coeffLow;
        *filterChain.get<1>().state = *coeffMid1;
        *filterChain.get<2>().state = *coeffMid2;
        *filterChain.get<3>().state = *coeffHigh;
    }

    void updateParams(const SamplerParameters& samplerSound, bool) override
    {
        updateParams(
            samplerSound.eqLowFreq->get(), samplerSound.eqHighFreq->get(), 
            samplerSound.eqLowGain->get(), samplerSound.eqMidGain->get(), samplerSound.eqHighGain->get()
        );
    }
    
    void process(juce::AudioBuffer<float>& buffer, int numSamples, int startSample = 0) override
    {
        juce::dsp::AudioBlock block{ buffer.getArrayOfWritePointers(), size_t(buffer.getNumChannels()), size_t(startSample), size_t(numSamples) };
        juce::dsp::ProcessContextReplacing context{ block };
        filterChain.process(context);
    }

    juce::Array<double> getMagnitudeForFrequencyArray(juce::Array<double> frequencies)
    {
        std::array filters{
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
            filter->state->getMagnitudeForFrequencyArray(frequencies.getRawDataPointer(), temp.getWritePointer(0), frequencies.size(), 48000);
            if (empty)
            {
                magnitudes.addFrom(0, 0, temp.getReadPointer(0), frequencies.size());
                empty = false;
            }
            else
            {
                for (int i = 0; i < temp.getNumSamples(); i++)
                    magnitudes.setSample(0, i, magnitudes.getSample(0, i) * temp.getSample(0, i));
            }
        }

        return juce::Array<double>{magnitudes.getReadPointer(0), frequencies.size()};
    }

private:
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    using FilterChain = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    static constexpr float Q{ 0.6f };  // Magic number I saw online for the response curve
    
    int sampleRate{ 0 };
    FilterChain filterChain;
};