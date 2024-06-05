/*
  ==============================================================================

    CustomSamplerSound.cpp
    Created: 5 Sep 2023 3:35:11pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerSound.h"

CustomSamplerSound::CustomSamplerSound(juce::AudioProcessorValueTreeState& apvts, PluginParameters::State& pluginState, const juce::AudioBuffer<float>& sample, int sampleRate) : 
    sample(sample), sampleRate(sampleRate),
    gain(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::MASTER_GAIN))),
    speedFactor(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::SPEED_FACTOR))),
    semitoneTuning(dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter(PluginParameters::SEMITONE_TUNING))),
    centTuning(dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter(PluginParameters::CENT_TUNING))),
    monoOutput(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::MONO_OUTPUT))),
    skipAntialiasing(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::SKIP_ANTIALIASING))),

    sampleStart(pluginState.sampleStart), sampleEnd(pluginState.sampleEnd),
    loopStart(pluginState.loopStart), loopEnd(pluginState.loopEnd),

    isLooping(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::IS_LOOPING))),
    loopingHasStart(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::LOOPING_HAS_START))),
    loopingHasEnd(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::LOOPING_HAS_END))),

    reverbEnabled(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::REVERB_ENABLED))),
    distortionEnabled(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::DISTORTION_ENABLED))),
    eqEnabled(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::EQ_ENABLED))),
    chorusEnabled(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::CHORUS_ENABLED))),

    reverbMix(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::REVERB_MIX))),
    reverbSize(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::REVERB_SIZE))),
    reverbDamping(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::REVERB_DAMPING))),
    reverbLows(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::REVERB_LOWS))),
    reverbHighs(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::REVERB_HIGHS))),
    reverbPredelay(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::REVERB_PREDELAY))),

    distortionMix(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::DISTORTION_MIX))),
    distortionDensity(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::DISTORTION_DENSITY))),
    distortionHighpass(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::DISTORTION_HIGHPASS))),

    eqLowGain(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::EQ_LOW_GAIN))),
    eqMidGain(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::EQ_MID_GAIN))),
    eqHighGain(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::EQ_HIGH_GAIN))),
    eqLowFreq(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::EQ_LOW_FREQ))),
    eqHighFreq(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::EQ_HIGH_FREQ))),

    chorusMix(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::CHORUS_MIX))),
    chorusRate(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::CHORUS_RATE))),
    chorusDepth(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::CHORUS_DEPTH))),
    chorusFeedback(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::CHORUS_FEEDBACK))),
    chorusCenterDelay(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PluginParameters::CHORUS_CENTER_DELAY))),

    playbackMode(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(PluginParameters::PLAYBACK_MODE))),
    fxOrder(dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter(PluginParameters::FX_PERM)))
{

    doPreprocess = PluginParameters::PREPROCESS_STEP;
    attackSmoothingSamples = PluginParameters::ATTACK_SMOOTHING;
    releaseSmoothingSamples = PluginParameters::RELEASE_SMOOTHING;
    crossfadeSamples = PluginParameters::CROSSFADING;
}

bool CustomSamplerSound::appliesToNote(int)
{
    return true;
}

bool CustomSamplerSound::appliesToChannel(int)
{
    return true;
}

PluginParameters::PLAYBACK_MODES CustomSamplerSound::getPlaybackMode() const
{
    return PluginParameters::getPlaybackMode(float(playbackMode->getIndex()));
}

std::array<PluginParameters::FxTypes, 4> CustomSamplerSound::getFxOrder() const
{
    return PluginParameters::paramToPerm(fxOrder->get());
}
