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
    PREPROCESSING,
    PLAYING, // this doubles as the loop start when that's enabled
    LOOPING,
    RELEASING, // for loop end portion
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

    /* Get the effective location of the sampler voice relative to the original sample, not precise in ADVANCED mode */
    int getEffectiveLocation();

    PluginParameters::PLAYBACK_MODES& getPlaybackMode()
    {
        return playbackMode;
    }

    VoiceState& getCurrentState()
    {
        return state;
    }

    std::unique_ptr<BufferPitcher>& getBufferPitcher(VoiceState state)
    {
        return state == RELEASING ? releaseBuffer : startBuffer;
    }

    int getCurrentSample() const
    {
        return currentSample;
    }

    float getSampleRateConversion() const
    {
        return sampleRateConversion;
    }

    int effectiveStart{ 0 }, effectiveEnd{ 0 };
private:
    CustomSamplerSound* sampleSound{ nullptr };
    float sampleRateConversion{ 0 };
    float noteFreq{ 0 };
    float velocity{ 0 };
    int pitchWheel{ 0 };
    bool isLooping{ false }, loopingHasStart{ false }, loopingHasEnd{ false };
    int sampleStart{ 0 }, sampleEnd{ 0 }, loopStart{ 0 }, loopEnd{ 0 };
    PluginParameters::PLAYBACK_MODES playbackMode{ PluginParameters::PLAYBACK_MODES::BASIC };
    int numChannels;

    VoiceState state{ STOPPED };
    int currentSample{ 0 }; // includes bufferPitcher->startDelay when playbackMode == ADVANCED, includes effectiveStart (note that effectiveStart is in original sample rate)
    juce::Array<float> previousSample;
    int noteDuration{ 0 };
    std::unique_ptr<BufferPitcher> startBuffer; // assumption is these have the same startDelay
    std::unique_ptr<BufferPitcher> releaseBuffer;

    bool midiReleased{ false };
    int midiReleasedSamples{ 0 };

    bool doStartStopSmoothing{ false };
    bool doCrossfadeSmoothing{ false };
    int startStopSmoothingSamples{ 0 };
    int crossfadeSmoothingSamples{ 0 };

    bool isSmoothingStart{ false };
    bool isSmoothingLoop{ false };
    bool isSmoothingRelease{ false };
    bool isSmoothingEnd{ false };

    int smoothingStartSample{ 0 };
    int smoothingLoopSample{ 0 };
    int smoothingReleaseSample{ 0 };
    int smoothingEndSample{ 0 };

    int preprocessingTotalSamples{ 0 };
    int preprocessingSample{ 0 };
};
