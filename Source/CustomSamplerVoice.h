/*
  ==============================================================================

    CustomSamplerVoice.h
    Created: 5 Sep 2023 3:35:03pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CustomSamplerSound.h"
#include "PitchShifter.h"
#include <RubberBandStretcher.h>

using namespace juce;

enum VoiceState
{
    STARTING,
    PLAYING,
    STOPPING,
    STOPPED
};

class CustomSamplerVoice : public SynthesiserVoice
{
public:
    CustomSamplerVoice(double sampleRate, int numChannels);
    ~CustomSamplerVoice();
    // Inherited via SynthesiserVoice
    bool canPlaySound(SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    int getEffectiveLocation();

    PluginParameters::PLAYBACK_MODES& getPlaybackMode()
    {
        return playbackMode;
    }

    VoiceState& getCurrentState()
    {
        return state;
    }

    BufferPitcher* getBufferPitcher()
    {
        return bufferPitcher;
    }

    int getCurrentSample()
    {
        return currentSample;
    }

    float getSampleRateConversion()
    {
        return sampleRateConversion;
    }
private:
    const int START_SAMPLES = 300;
    const int STOP_SAMPLES = 3000;

    CustomSamplerSound* sampleSound{ nullptr };
    float sampleRateConversion{ 0 };
    float noteFreq{ 0 };
    float velocity{ 0 };
    int pitchWheel{ 0 };
    PluginParameters::PLAYBACK_MODES playbackMode{ PluginParameters::PLAYBACK_MODES::BASIC };

    VoiceState state{ STOPPED };
    int smoothingSample{ 0 };

    BufferPitcher* bufferPitcher{ nullptr };
    int numChannels;
    int currentSample{ 0 };
};
