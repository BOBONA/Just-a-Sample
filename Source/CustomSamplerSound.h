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
    CustomSamplerSound(AudioProcessorValueTreeState& apvts, AudioBuffer<float>& sample, int sampleRate);
    // Inherited via SynthesiserSound
    bool appliesToNote(int midiNoteNumber) override;
    bool appliesToChannel(int midiChannel) override;

    PluginParameters::PLAYBACK_MODES getPlaybackMode();

    AudioBuffer<float>& sample;
    int sampleRate;
    float speedFactor;
    juce::Value gain, semitoneTuning, centTuning;
    juce::Value sampleStart, sampleEnd, 
        isLooping, loopingHasStart, loopingHasEnd, loopStart, loopEnd;
    bool doPreprocess;
    bool doStartStopSmoothing;
    bool doCrossfadeSmoothing;
    int startStopSmoothingSamples;
    int crossfadeSmoothingSamples;

    juce::Value reverbMix, reverbSize, reverbDamping, reverbParam1, reverbParam2, reverbParam3;
    juce::Value distortionMix, distortionOutput, distortionDensity, distortionHighpass;
    juce::Value eqLowGain, eqMidGain, eqHighGain;
private:
    juce::Value playbackMode;
};