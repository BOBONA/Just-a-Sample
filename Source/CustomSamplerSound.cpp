/*
  ==============================================================================

    CustomSamplerSound.cpp
    Created: 5 Sep 2023 3:35:11pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerSound.h"

CustomSamplerSound::CustomSamplerSound(AudioProcessorValueTreeState& apvts, AudioBuffer<float>& sample, int sampleRate) : 
    sample(sample), sampleRate(sampleRate)
{
    gain = apvts.getParameterAsValue(PluginParameters::MASTER_GAIN);
    semitoneTuning = apvts.getParameterAsValue(PluginParameters::SEMITONE_TUNING);
    centTuning = apvts.getParameterAsValue(PluginParameters::CENT_TUNING);
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

    reverbMix = apvts.getParameterAsValue(PluginParameters::REVERB_MIX);
    reverbSize = apvts.getParameterAsValue(PluginParameters::REVERB_SIZE);
    reverbDamping = apvts.getParameterAsValue(PluginParameters::REVERB_DAMPING);
    reverbParam1 = apvts.getParameterAsValue(PluginParameters::REVERB_PARAM1);
    reverbParam2 = apvts.getParameterAsValue(PluginParameters::REVERB_PARAM2);
    reverbParam3 = apvts.getParameterAsValue(PluginParameters::REVERB_PARAM3);

    distortionMix = apvts.getParameterAsValue(PluginParameters::DISTORTION_MIX);
    distortionOutput = apvts.getParameterAsValue(PluginParameters::DISTORTION_OUTPUT);
    distortionDensity = apvts.getParameterAsValue(PluginParameters::DISTORTION_DENSITY);
    distortionHighpass = apvts.getParameterAsValue(PluginParameters::DISTORTION_HIGHPASS);

    eqLowGain = apvts.getParameterAsValue(PluginParameters::EQ_LOW_GAIN);
    eqMidGain = apvts.getParameterAsValue(PluginParameters::EQ_MID_GAIN);
    eqHighGain = apvts.getParameterAsValue(PluginParameters::EQ_HIGH_GAIN);
    eqLowFreq = apvts.getParameterAsValue(PluginParameters::EQ_LOW_FREQ);
    eqHighFreq = apvts.getParameterAsValue(PluginParameters::EQ_HIGH_FREQ);
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
    return PluginParameters::getPlaybackMode(playbackMode.getValue());
}
