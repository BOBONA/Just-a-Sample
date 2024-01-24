/*
  ==============================================================================

    CustomSamplerSound.h
    Created: 5 Sep 2023 3:35:11pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../PluginParameters.h"

using namespace juce;

class CustomSamplerSound : public SynthesiserSound
{
public:
    CustomSamplerSound(AudioProcessorValueTreeState& apvts, AudioBuffer<float>& sample, int sampleRate);
    // Inherited via SynthesiserSound
    bool appliesToNote(int midiNoteNumber) override;
    bool appliesToChannel(int midiChannel) override;

    PluginParameters::PLAYBACK_MODES getPlaybackMode();
    std::array<PluginParameters::FxTypes, 4> getFxOrder();

    AudioBuffer<float>& sample;
    int sampleRate;
    juce::Value gain, semitoneTuning, centTuning, speedFactor, monoOutput, formantPreserved, skipAntialiasing;
    juce::Value sampleStart, sampleEnd, 
        isLooping, loopingHasStart, loopingHasEnd, loopStart, loopEnd;
    bool doPreprocess;
    bool doStartStopSmoothing;
    bool doCrossfadeSmoothing;
    int startStopSmoothingSamples;
    int crossfadeSmoothingSamples;

    juce::Value reverbEnabled, distortionEnabled, eqEnabled, chorusEnabled;
    juce::Value reverbMix, reverbSize, reverbDamping, reverbLows, reverbHighs, reverbPredelay;
    juce::Value distortionMix, distortionDensity, distortionHighpass;
    juce::Value eqLowGain, eqMidGain, eqHighGain, eqLowFreq, eqHighFreq;
    juce::Value chorusRate, chorusDepth, chorusFeedback, chorusCenterDelay, chorusMix;
private:
    juce::Value playbackMode;
    juce::Value fxOrder;
};