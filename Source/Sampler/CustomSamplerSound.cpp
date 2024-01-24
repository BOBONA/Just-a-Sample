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
    speedFactor = apvts.getParameterAsValue(PluginParameters::SPEED_FACTOR);
    monoOutput = apvts.getParameterAsValue(PluginParameters::MONO_OUTPUT);
    formantPreserved = apvts.getParameterAsValue(PluginParameters::FORMANT_PRESERVED);
    skipAntialiasing = apvts.getParameterAsValue(PluginParameters::SKIP_ANTIALIASING);

    sampleStart = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_START, apvts.undoManager);
    sampleEnd = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_END, apvts.undoManager);
    isLooping = apvts.getParameterAsValue(PluginParameters::IS_LOOPING);
    loopingHasStart = apvts.state.getPropertyAsValue(PluginParameters::LOOPING_HAS_START, apvts.undoManager);
    loopingHasEnd = apvts.state.getPropertyAsValue(PluginParameters::LOOPING_HAS_END, apvts.undoManager);
    loopStart = apvts.state.getPropertyAsValue(PluginParameters::LOOP_START, apvts.undoManager);
    loopEnd = apvts.state.getPropertyAsValue(PluginParameters::LOOP_END, apvts.undoManager);
    playbackMode = apvts.getParameterAsValue(PluginParameters::PLAYBACK_MODE);

    doPreprocess = PluginParameters::PREPROCESS_STEP;
    doStartStopSmoothing = PluginParameters::DO_START_STOP_SMOOTHING;
    doCrossfadeSmoothing = PluginParameters::DO_CROSSFADE_SMOOTHING;
    startStopSmoothingSamples = PluginParameters::START_STOP_SMOOTHING;
    crossfadeSmoothingSamples = PluginParameters::CROSSFADE_SMOOTHING;

    reverbEnabled = apvts.getParameterAsValue(PluginParameters::REVERB_ENABLED);
    reverbMix = apvts.getParameterAsValue(PluginParameters::REVERB_MIX);
    reverbSize = apvts.getParameterAsValue(PluginParameters::REVERB_SIZE);
    reverbDamping = apvts.getParameterAsValue(PluginParameters::REVERB_DAMPING);
    reverbLows = apvts.getParameterAsValue(PluginParameters::REVERB_LOWS);
    reverbHighs = apvts.getParameterAsValue(PluginParameters::REVERB_HIGHS);
    reverbPredelay = apvts.getParameterAsValue(PluginParameters::REVERB_PREDELAY);

    distortionEnabled = apvts.getParameterAsValue(PluginParameters::DISTORTION_ENABLED);
    distortionMix = apvts.getParameterAsValue(PluginParameters::DISTORTION_MIX);
    distortionDensity = apvts.getParameterAsValue(PluginParameters::DISTORTION_DENSITY);
    distortionHighpass = apvts.getParameterAsValue(PluginParameters::DISTORTION_HIGHPASS);

    eqEnabled = apvts.getParameterAsValue(PluginParameters::EQ_ENABLED);
    eqLowGain = apvts.getParameterAsValue(PluginParameters::EQ_LOW_GAIN);
    eqMidGain = apvts.getParameterAsValue(PluginParameters::EQ_MID_GAIN);
    eqHighGain = apvts.getParameterAsValue(PluginParameters::EQ_HIGH_GAIN);
    eqLowFreq = apvts.getParameterAsValue(PluginParameters::EQ_LOW_FREQ);
    eqHighFreq = apvts.getParameterAsValue(PluginParameters::EQ_HIGH_FREQ);

    chorusEnabled = apvts.getParameterAsValue(PluginParameters::CHORUS_ENABLED);
    chorusRate = apvts.getParameterAsValue(PluginParameters::CHORUS_RATE);
    chorusDepth = apvts.getParameterAsValue(PluginParameters::CHORUS_DEPTH);
    chorusFeedback = apvts.getParameterAsValue(PluginParameters::CHORUS_FEEDBACK);
    chorusCenterDelay = apvts.getParameterAsValue(PluginParameters::CHORUS_CENTER_DELAY);
    chorusMix = apvts.getParameterAsValue(PluginParameters::CHORUS_MIX);

    fxOrder = apvts.getParameterAsValue(PluginParameters::FX_PERM);
}

bool CustomSamplerSound::appliesToNote(int)
{
    return true;
}

bool CustomSamplerSound::appliesToChannel(int)
{
    return true;
}

PluginParameters::PLAYBACK_MODES CustomSamplerSound::getPlaybackMode()
{
    return PluginParameters::getPlaybackMode(playbackMode.getValue());
}

std::array<PluginParameters::FxTypes, 4> CustomSamplerSound::getFxOrder()
{
    return PluginParameters::paramToPerm(fxOrder.getValue());
}