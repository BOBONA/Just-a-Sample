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
    BUFFERING, // some preprocessing done during the first few render calls
    PLAYING, // this doubles as the loop start when that's enabled
    LOOPING,
    RELEASING, // for loop end portion
    STOPPING, // while smoothing stop
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

    /* sampleLocation corresponds to currentSample, not the actual sample index */
    float getSample(int channel, int sampleLocation, VoiceState voiceState);

    /* Starts a smoothing process to prevent clicks/pops */
    void startSmoothing(float smoothingInitial);

    PluginParameters::PLAYBACK_MODES& getPlaybackMode()
    {
        return playbackMode;
    }

    VoiceState& getCurrentState()
    {
        return state;
    }

    /* Helper functions for dealing with the bufferPitchers */
    std::unique_ptr<BufferPitcher>& getBufferPitcher();
    std::unique_ptr<BufferPitcher>& getBufferPitcher(VoiceState voiceState);
    void instantiateBufferPitcher(bool newSound, std::unique_ptr<BufferPitcher>& bufferPitcher, int sampleStart, int sampleEnd);

    int getCurrentSample()
    {
        return currentSample;
    }

    float getSampleRateConversion()
    {
        return sampleRateConversion;
    }

    int effectiveStart{ 0 }, effectiveEnd{ 0 };
private:
    static const int SMOOTHING_SAMPLES{ 550 };

    CustomSamplerSound* sampleSound{ nullptr };
    float sampleRateConversion{ 0 };
    float noteFreq{ 0 };
    float velocity{ 0 };
    int pitchWheel{ 0 };
    bool isLooping{ false }, loopingHasStart{ false }, loopingHasEnd{ false };
    int sampleStart{ 0 }, sampleEnd{ 0 }, loopStart{ 0 }, loopEnd{ 0 };
    PluginParameters::PLAYBACK_MODES playbackMode{ PluginParameters::PLAYBACK_MODES::BASIC };

    VoiceState state{ STOPPED };
    bool isSmoothing{ false };
    int smoothingSample{ 0 }; // goes from 0 to SMOOTHING_SAMPLES - 1
    float smoothingInitial{ 0 }; // to smooth from a starting value

    std::unique_ptr<BufferPitcher> startBuffer;
    std::unique_ptr<BufferPitcher> loopBuffer;
    std::unique_ptr<BufferPitcher> releaseBuffer;
    int numChannels;
    int currentSample{ 0 }; // includes bufferPitcher->startDelay when playbackMode == ADVANCED

    int bufferingSamples{ 0 }; // this is used to keep track of how many samples have been rendered in the BUFFERING state
    VoiceState bufferingState{ PLAYING }; // this is to keep track of what we are buffering
};
