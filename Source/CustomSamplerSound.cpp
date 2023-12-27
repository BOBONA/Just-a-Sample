/*
  ==============================================================================

    CustomSamplerSound.cpp
    Created: 5 Sep 2023 3:35:11pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerSound.h"

CustomSamplerSound::CustomSamplerSound(AudioProcessorValueTreeState& apvts, AudioBuffer<float>& sample, int sampleRate, float& baseFreq) : 
    sample(sample), sampleRate(sampleRate), baseFreq(baseFreq)
{
    gain = apvts.getParameterAsValue(PluginParameters::MASTER_GAIN);
    sampleStart = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_START, apvts.undoManager);
    sampleEnd = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_END, apvts.undoManager);
    isLooping = apvts.getParameterAsValue(PluginParameters::IS_LOOPING);
    loopingHasStart = apvts.state.getPropertyAsValue(PluginParameters::LOOPING_HAS_START, apvts.undoManager);
    loopingHasEnd = apvts.state.getPropertyAsValue(PluginParameters::LOOPING_HAS_END, apvts.undoManager);
    loopStart = apvts.state.getPropertyAsValue(PluginParameters::LOOP_START, apvts.undoManager);
    loopEnd = apvts.state.getPropertyAsValue(PluginParameters::LOOP_END, apvts.undoManager);
    playbackMode = apvts.getParameterAsValue(PluginParameters::PLAYBACK_MODE);
    speedFactor = PluginParameters::SPEED_FACTOR;
    doPreprocess = PluginParameters::PREPROCESS_STEP;
    doStartStopSmoothing = PluginParameters::DO_START_STOP_SMOOTHING;
    doCrossfadeSmoothing = PluginParameters::DO_CROSSFADE_SMOOTHING;
    startStopSmoothingSamples = PluginParameters::START_STOP_SMOOTHING;
    crossfadeSmoothingSamples = PluginParameters::CROSSFADE_SMOOTHING;
}

bool CustomSamplerSound::appliesToNote(int midiNoteNumber)
{
    return true;
}

bool CustomSamplerSound::appliesToChannel(int midiChannel)
{
    return true;
}

PluginParameters::PLAYBACK_MODES CustomSamplerSound::getPlaybackMode()
{
    return static_cast<PluginParameters::PLAYBACK_MODES>(int(playbackMode.getValue()));
}
