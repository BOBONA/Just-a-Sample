/*
  ==============================================================================

    CustomSamplerSound.h
    Created: 5 Sep 2023 3:35:11pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginParameters.h"

using namespace juce;

class CustomSamplerSound : public SynthesiserSound
{
public:
    CustomSamplerSound(AudioProcessorValueTreeState& apvts, AudioBuffer<float>& sample, int sampleRate, float& baseFreq);
    // Inherited via SynthesiserSound
    bool appliesToNote(int midiNoteNumber) override;
    bool appliesToChannel(int midiChannel) override;

    PluginParameters::PLAYBACK_MODES getPlaybackMode();

    AudioBuffer<float>& sample;
    int sampleRate;
    float& baseFreq;
    juce::Value sampleStart, sampleEnd,
        isLooping, loopingHasStart, loopingHasEnd, loopStart, loopEnd;
private:
    juce::Value playbackMode;
};