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

/** A class defining all parameters for a note played by CustomSamplerVoice.cpp */
class SamplerParameters final
{
public:
    SamplerParameters(const juce::AudioProcessorValueTreeState& apvts, PluginParameters::State& pluginState, const juce::AudioBuffer<float>& sample, int sampleRate);

    void sampleChanged(int newSampleRate);

    /** Fetch the playback mode, as the proper enum type */
    PluginParameters::PLAYBACK_MODES getPlaybackMode() const;

    /** Fetch the sound's FX chain permutation */
    std::array<PluginParameters::FxTypes, 4> getFxOrder() const;

    /** The sound to play */
    const juce::AudioBuffer<float>& sample;
    int sampleRate;

    /** Playback details */
    juce::AudioParameterFloat* gain, * speedFactor;
    juce::AudioParameterInt* semitoneTuning, * centTuning, * attack, * release;
    juce::AudioParameterBool* monoOutput, * skipAntialiasing, * skipLowpass, * isLooping, * loopingHasStart, * loopingHasEnd;
    ListenableAtomic<int>& sampleStart, & sampleEnd, & loopStart, & loopEnd;

    /** Smoothing configuration */
    float crossfadeSamples;

    /** FX parameters */
    juce::AudioParameterBool* reverbEnabled, * distortionEnabled, * eqEnabled, * chorusEnabled;
    juce::AudioParameterFloat* reverbMix, * reverbSize, * reverbDamping, * reverbLows, * reverbHighs, * reverbPredelay;
    juce::AudioParameterFloat* distortionMix, * distortionDensity, * distortionHighpass;
    juce::AudioParameterFloat* eqLowGain, * eqMidGain, * eqHighGain, * eqLowFreq, * eqHighFreq;
    juce::AudioParameterFloat* chorusMix, * chorusRate, * chorusDepth, * chorusFeedback, * chorusCenterDelay;

private:
    juce::AudioParameterChoice* playbackMode;
    juce::AudioParameterInt* fxOrder;
};